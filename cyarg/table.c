#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "yargtype.h"

#define TABLE_MAX_LOAD 3 / 4

typedef struct TableEntry
{
    struct TableEntry *next_;
    ObjString* key_;
    Value value_; // todo should be optional Value, ValueCell or nothing
} TableEntry;

typedef struct TableBlock
{
    struct TableBlock *next_;
    TableEntry entries[];
} TableBlock;

typedef struct Table
{
    int size_;
    TableBlock *blockList;
    TableEntry *free_;
    int listsSize_; // only needed for speed optimisation
    TableEntry *lists[];
} Table;

static void expand(Table *table);
static TableEntry* findEntry(Table *table, ObjString const *key);
static TableEntry* findOrCreateEntry(Table *table, ObjString const *key);

void initTable(int self[], int size)
{
    Table *table = (Table *) self;
    assert(size >= sizeof (Table) + sizeof table->lists[0]);
    memset(table, 0, size);
    table->size_ = size;
    table->listsSize_ = (size - sizeof (Table)) / sizeof table->lists[0];
    table->free_ = 0;
    table->blockList = 0;
}

void freeTable(int self[]) {
    Table *table = (Table *) self;

    TableBlock *tableBlock = table->blockList;
    while (tableBlock != 0)
    {
        TableBlock *next = tableBlock->next_;
        reallocate(tableBlock, sizeof (TableBlock) +  sizeof (TableEntry) * (table->listsSize_ + 1), 0);
        tableBlock = next;
    }
    initTable(self, table->size_);
}

bool tableGet(int self[], ObjString* key, Value* value)
{
    Table *table = (Table *) self;

    TableEntry* entry = findEntry(table, key);
    if (entry == NULL) return false;

    *value = entry->value_;
    return true;
}

bool tableSet(int self[], ObjString* key, Value value)
{
    Table *table = (Table *) self;

    TableEntry* entry = findOrCreateEntry(table, key);
    bool isNewKey = entry->key_ == NULL;

    entry->key_ = key;
    entry->value_ = value;
    return isNewKey;
}

//bool tableDelete(int self[], ObjString* key)
//{
//    Table *table = (Table *) self;
//
//    int i = key->hash % table->listsSize_;
//
//    TableEntry **preceder = &table->lists[i];
//    TableEntry *r = table->lists[i];
//
//    while (r != 0 && strcmp(key->chars, r->key_->chars) != 0)
//    {
//        preceder = &r->next_;
//        r = r->next_;
//    }
//
//    if (r == 0)
//    {
//        return false;
//    }
//
//    *preceder = r->next_;
//    r->next_ = table->free_;
//    table->free_ = r;
//
//    return true;
//}

void tableAddAll(int sFrom[], int sTo[])
{
    Table *from = (Table *) sFrom;

    for (int i = 0; i < from->listsSize_; i++)
    {
        TableEntry *r = from->lists[i];

        while (r != 0)
        {
            tableSet(sTo, r->key_, r->value_);
            r = r->next_;
        }
    }
}

ObjString* tableFindString(int self[], const char *chars, int length, uint32_t hash)
{
    Table *table = (Table *) self;
    int i = hash % table->listsSize_;

    TableEntry *r = table->lists[i];

    while (r != 0 && (length != r->key_->length || memcmp(chars, r->key_->chars, length) != 0))
    {
        r = r->next_;
    }
    if (r != 0)
    {
        return r->key_;
    }
    else
    {
        return 0;
    }
}

void tableRemoveWhite(int self[])
{
    Table *table = (Table *) self;

    for (int i = 0; i < table->listsSize_; i++)
    {
        TableEntry *r = table->lists[i];
        TableEntry **preceding = &table->lists[i];
        while (r != 0)
        {
            if (!r->key_->obj.isMarked)
            {
                TableEntry *next = r->next_;
                *preceding = next;
                r->next_ = table->free_;
                table->free_ = r;
                r = next;
            }
            else
            {
                preceding = &r->next_;
                r = r->next_;
            }
        }
    }
}

void markTable(int self[])
{
    Table *table = (Table *) self;

    for (int i = 0; i < table->listsSize_; i++)
    {
        TableEntry *r = table->lists[i];

        while (r != 0)
        {
            markValue(r->value_);
            r = r->next_;
        }
    }
}

static void expand(Table *table)
{
    TableBlock *list = table->blockList;
    table->blockList = (TableBlock *) reallocate(NULL, 0, sizeof (TableBlock) +  sizeof (TableEntry) * (table->listsSize_ + 1)); // ensure there are at lease two TableEntrys
    table->blockList->next_ = list;
    table->free_ = table->blockList->entries;

    TableEntry *te = table->free_;
    for (int i = 0; i < table->listsSize_; i++)
    {
        te->next_ = te + 1;
        te = te->next_;
    }
    te->next_ = 0;
}

static TableEntry* findEntry(Table *table, ObjString const *key)
{
    int i = key->hash % table->listsSize_;

    TableEntry *r = table->lists[i];

    while (r != 0 && (key->length != r->key_->length || memcmp(key->chars, r->key_->chars, key->length) != 0))
    {
        r = r->next_;
    }
    return r;
}

static TableEntry* findOrCreateEntry(Table *table, ObjString const *key)
{
    int i = key->hash % table->listsSize_;

    TableEntry *r = table->lists[i];

    while (r != 0 && r->key_ != key)
    {
        r = r->next_;
    }
    if (r == 0)
    {
        if (table->free_ == 0)
        {
            expand(table);
        }
        assert(table->free_ != 0); // todo - Panic
        r = table->free_;
        table->free_ = r->next_;

        r->next_ = table->lists[i];
        table->lists[i] = r;

        r->key_ = 0;
    }
    return r;
}

void initCellTable(ValueCellTable* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeCellTable(ValueCellTable* table) {
    FREE_ARRAY(EntryCell, table->entries, table->capacity);
    initCellTable(table);
}

static EntryCell* findCellEntry(EntryCell* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash & (capacity - 1);
    EntryCell* tombstone = NULL;

    for (;;) {
        EntryCell* entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->cell.value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool tableCellGet(ValueCellTable* table, ObjString* key, ValueCell* value) {
    if (table->count == 0) return false;

    EntryCell* entry = findCellEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->cell;
    return true;
}

bool tableCellGetPlace(ValueCellTable* table, ObjString* key, ValueCell** place) {
    if (table->count == 0) return false;

    EntryCell* entry = findCellEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *place = &entry->cell;
    return true;
}

static void adjustCellCapacity(ValueCellTable* table, int capacity) {
    EntryCell* entries = ALLOCATE(EntryCell, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].cell.value = NIL_VAL;
        entries[i].cell.cellType = NULL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        EntryCell* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        EntryCell* dest = findCellEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->cell= entry->cell;
        table->count++;
    }

    FREE_ARRAY(EntryCell, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableCellSet(ValueCellTable* table, ObjString* key, ValueCell cell) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCellCapacity(table, capacity);
    }

    EntryCell* entry = findCellEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->cell.value)) table->count++;

    entry->key = key;
    entry->cell = cell;
    return isNewKey;
}

bool tableCellDelete(ValueCellTable* table, ObjString* key) {
    if (table->count == 0) return false;

    // Find the entry.
    EntryCell* entry = findCellEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->cell.value = BOOL_VAL(true);
    entry->cell.cellType = NULL;
    return true;
}

void tableCellAddAll(ValueCellTable* from, ValueCellTable* to) {
    for (int i = 0; i < from->capacity; i++) {
        EntryCell* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableCellSet(to, entry->key, entry->cell);
        }
    }
}

ObjString* tableCellFindString(ValueCellTable* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash & (table->capacity - 1);
    for (;;) {
        EntryCell* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->cell.value)) return NULL;
        } else if (entry->key->length == length &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

void tableCellRemoveWhite(ValueCellTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        EntryCell* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableCellDelete(table, entry->key);
        }
    }
}

void markCellTable(ValueCellTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        EntryCell* entry = &table->entries[i];
        if (entry->key != NULL) {
            markObject((Obj*)entry->key);
            markValueCell(&entry->cell);
        }
    }
}

void printCellTable(ValueCellTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        EntryCell* entry = &table->entries[i];
        if (entry->key != NULL) {
            printObject(OBJ_VAL((Obj*)entry->key));
            FPRINTMSG(stderr, ":::");
            printValue(entry->cell.value);
            FPRINTMSG(stderr, ":::");
            printType(stderr, entry->cell.cellType);
            FPRINTMSG(stderr, "\n");
        }
    }
}
