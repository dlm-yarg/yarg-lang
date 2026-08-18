//
//  object_store.h
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//
#ifndef cyarg_object_store_h
#define cyarg_object_store_h

#include "object_type.h"


void osInit(void); // first item in store - ObjPtr == 0, is an ObjNil

// osExtend() is only called from the VM when all threads and interrupts are in known states
// Obj *s may change - ObjPtrs are immutable
void osExtend(void); // defrags, rebalances and/or extends the pools; it may nop.

ObjPtr osAlloc(ObjType, ObjSize);
void osRealloc(ObjPtr, ObjSize);
void osFree(ObjPtr); // called on uncaptured objects when a stack frame collapses. if arg has been lazy copied the copy takes ownership of the Obj. nop if ObjPtr is a ROM object

void osNoGc(ObjPtr); // arg not liable for GC. Call osGcOk() when eligible for GC. Use to ensure Obj * is valid over osAlloc()/osRealloc(), for long lived objects to optimise GC or placed memory
void osGcOk(ObjPtr); // arg is liable for GC.
ObjPtr osCopy(ObjPtr); // create a lazy copy (or optionally a full copy e.g. for small objects or debugging)
ObjPtr osStoreRomObject(void *, ObjType, ObjSize);

void osGc(void); // decimated GC callibrated to take < ~3 ms. VM calls this after every OP_LOOP, OP_CALL, OP_RETURN. May nop.

// do not leak the returned Obj * past the next } (end of block) without locking/unlocking - osNoGc()/osGcOk()
PtrEntry const *osEntry(ObjPtr);
ObjType osType(ObjPtr);
void const *osDeref(ObjPtr); // may be modified if it is know not to be a copy e.g. immediately after alloc
void *osDerefAndModify(ObjPtr); // if arg is a lazy copy object it will be duplicated unless the original was freed, ROM objects are duplicated

ObjPtr osStoreCString(char const *, ArrayItemCount);
ObjPtr osStoreRomCString(char const *); // null terminated
char const *osStringAsCString(ObjPtr); // null terminated
void deleteString(ObjPtr); // called from GC

#endif
