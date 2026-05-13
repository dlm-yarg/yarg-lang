//
//  object_store.h
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//
#ifndef cyarg_object_store_h
#define cyarg_object_store_h

#include "object_type.h"

typedef struct Obj Obj;

void osInit(void); // first item in store - ObjPtr == 0, is an ObjNil

// osExtend() is only called from the VM when all threads and interrupts are in known states
// Obj *s may change - ObjPtrs are immutable
void osExtend(void); // defrags, rebalances and/or extends the pools; it may nop.

ObjPtr osAlloc(ObjSize);
void osNoGc(ObjPtr); // arg not liable for GC. Call osGcOk() when eligible for GC.
void osGcOk(ObjPtr); // arg is liable for GC.
void osRealloc(ObjPtr, ObjSize);
void osFree(ObjPtr); // called on uncaptured objects when a stack frame collapses. if arg has been lazy copied the copy takes ownership of the Obj.
ObjPtr osCopy(ObjPtr); // create a lazy copy (only one copy is possible)
void osModify(ObjPtr); // if arg is a lazy copy object it will be duplicated unless the original was freed, otherwise nop.
void osGc(void); // decimated GC callibrated to take < ~3 ms. VM calls this after every OP_LOOP, OP_CALL, OP_RETURN. May nop.

Obj *osDeref(ObjPtr); // do not retain the returned Obj * outside the current scope

#endif
