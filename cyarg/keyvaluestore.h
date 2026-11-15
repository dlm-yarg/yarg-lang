//
//  keyvaluestore.h
//  gh.yarg-lang
//
//  Created by dlm on 03/11/2026.
//
#ifndef cyarg_keyvaluestore_h
#define cyarg_keyvaluestore_h

#include <stdbool.h>

typedef struct ValueCell ValueCell;
typedef struct Value Value;
typedef struct ObjString ObjString;
typedef struct ValueCellStoreNode ValueCellStoreNode;

typedef struct KeyValueStore {
    ValueCellStoreNode *root;
} KeyValueStore;

typedef struct ObjString ObjString;

void keyValueStoreInit(KeyValueStore *);
int keyValueStoreCount(const KeyValueStore *);
void keyValueStoreFree(KeyValueStore *); // vm_mutex_enter_blocking()/vm_mutex_exit() must be called around this function
bool keyValueStoreGet(const KeyValueStore *, ObjString *key, ValueCell *);
bool keyValueStoreGetValue(const KeyValueStore *, ObjString *key, Value *);
bool keyValueStorePlace(const KeyValueStore *, ObjString *key, ValueCell **);
bool keyValueStoreSet(KeyValueStore *, ObjString *key, const ValueCell *);
bool keyValueStoreSetValue(KeyValueStore *, ObjString *key, const Value *);
void keyValueStoreAddAll(const KeyValueStore *from, KeyValueStore *to);
void keyValueStoreMark(KeyValueStore *);

#endif // !cyarg_keyvaluestore_h
