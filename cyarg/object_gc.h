#ifndef cyarg_object_gc_h
#define cyarg_object_gc_h

#include "value.h"
#include "vm_mutex.h"

#define TR_ENTRY        size_t _tr_entry_ = tempRootDebugSize()
#define TR_EXIT(EX)     do { assert(tempRootDebugSize() - _tr_entry_ - (EX) == 0); } while (0)
//#define TR_ENTRY
//#define TR_EXIT(EX)

typedef struct Obj Obj;

// GC startup
void vm_objects_init(vm_mutex* mutex);

void gc_register_object(Obj* object);

// shutdown, freeing all GC managed objects
void vm_objects_free();

// API for various GC features: temporary root management & pinning of objects

// temporary root management for the garbage collector
void tempRootPush(Value value);
void tempObjRootPush(Obj* object);
Value tempRootPop();
Value tempRootPopBelow(int);

void pinObj(Obj* object);

size_t tempRootDebugSize(void);

#endif // cyarg_object_gc_h
