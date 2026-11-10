//
//  stringstore.h
//  gh.yarg-lang
//
//  Created by dlm on 03/11/2026.
//

#include "yargobject.h"

typedef struct StringStore {
    ObjString *root;
} StringStore;

void stringStoreInit(StringStore *);
void stringStoreFree(StringStore *);
ObjString *stringStoreStore(StringStore *, ObjString *); // return will be the stored string or an identical one
ObjString *stringStoreGet(StringStore *, char const *);
void stringStoreDeleteUnmarked(StringStore *);
