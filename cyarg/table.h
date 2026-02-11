#ifndef cyarg_table_h
#define cyarg_table_h

#include "common.h"
#include "value.h"

typedef int ValueTable;

void initTable(ValueTable [], int sizeInChars);
void freeTable(ValueTable []);
bool tableGet(ValueTable [], ObjString* key, Value* value);
bool tableSet(ValueTable [], ObjString* key, Value value); // Value const *?
void tableAddAll(ValueTable *from, ValueTable []);
ObjString* tableFindString(ValueTable [], const char* chars, int length, uint32_t hash);
void tableRemoveWhite(ValueTable []);
void markTable(ValueTable []);

typedef struct {
    ObjString* key;
    ValueCell cell;
} EntryCell;

typedef struct {
    int count;
    int capacity;
    EntryCell* entries;
} ValueCellTable;

void initCellTable(ValueCellTable* table);
void freeCellTable(ValueCellTable* table);
bool tableCellGet(ValueCellTable* table, ObjString* key, ValueCell* value);
bool tableCellGetPlace(ValueCellTable* table, ObjString* key, ValueCell** place);
bool tableCellSet(ValueCellTable* table, ObjString* key, ValueCell value);
bool tableCellDelete(ValueCellTable* table, ObjString* key);
void tableCellAddAll(ValueCellTable* from, ValueCellTable* to);
ObjString* tableCellFindString(ValueCellTable* table, const char* chars, int length, uint32_t hash);
void tableCellRemoveWhite(ValueCellTable* table);
void markCellTable(ValueCellTable* table);
void printCellTable(ValueCellTable* table);

#endif
