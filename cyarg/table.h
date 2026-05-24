#ifndef cyarg_table_h
#define cyarg_table_h

#include "value.h"

typedef struct {
    ObjPtr key;
    ObjPtr value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} ValueTable;

void initTable(ValueTable* table);
void freeTable(ValueTable* table);
bool tableGet(ValueTable* table, ObjPtr key, ObjPtr* value);
bool tableSet(ValueTable* table, ObjPtr key, ObjPtr value);
bool tableDelete(ValueTable* table, ObjString* key);
void tableAddAll(ValueTable* from, ValueTable* to);
ObjString* tableFindString(ValueTable* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(ValueTable* table);
void markTable(ValueTable* table);

typedef struct {
    ObjPtr key;
    ObjPtr cell;
} EntryCell;

typedef struct {
    int count;
    int capacity;
    EntryCell* entries;
} ValueCellTable;

void initCellTable(ValueCellTable* table);
void freeCellTable(ValueCellTable* table);
bool tableCellGet(ValueCellTable* table, ObjPtr key, ObjPtr* value);
bool tableCellGetPlace(ValueCellTable* table, ObjString* key, ObjPtr* place);
bool tableCellSet(ValueCellTable* table, ObjPtr key, ObjPtr value);
bool tableCellDelete(ValueCellTable* table, ObjPtr key);
void tableCellAddAll(ValueCellTable* from, ValueCellTable* to);
ObjString* tableCellFindString(ValueCellTable* table, const char* chars, int length, uint32_t hash);
void tableCellRemoveWhite(ValueCellTable* table);
void markCellTable(ValueCellTable* table);
void printCellTable(ValueCellTable* table);

#endif
