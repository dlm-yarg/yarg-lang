//
//  object_store.c
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//

#include "object_store.h"
#include "dynamic_array.h"

static DynamicArray osPointers;

// these bracket calls to alloc, owned, free, copy, modify and gc
static void lock(void);
static void unlock(void);


void osInit(void) {
    
}

void osExtend(void){}

ObjPtr osAlloc(ObjSize){return 0;}

void osNoGc(ObjPtr){}

void osGcOk(ObjPtr){}

void osRealloc(ObjPtr, ObjSize){}

void osFree(ObjPtr){}

ObjPtr osCopy(ObjPtr){return 0;}

void osModify(ObjPtr){}

void osGc(void){}

Obj *osDeref(ObjPtr){return 0;}

void lock(void){}

void unlock(void){}
