//
//  objroutine.h
//  gh.yarg-lang
//
//  Created by dlm on 20/11/2026.
//

#ifndef cyarg_objroutine_h
#define cyarg_objroutine_h

#include "value.h"
#include "value_cell.h"
#include "object_dyn_array.h"
#include "vm_mutex.h"

#define FRAMES_MAX 20
#define SLICE_MAX 64

typedef struct ObjRoutine ObjRoutine;
typedef struct ObjClosure ObjClosure;
typedef struct ObjUpvalue ObjUpvalue;
typedef struct ObjStackSlice ObjStackSlice;

typedef enum {
    EXEC_UNBOUND,
    EXEC_RUNNING,
    EXEC_SUSPENDED,
    EXEC_CLOSED,
    EXEC_ERROR
} ExecState;

typedef bool (*AddSliceFn)(ObjRoutine* routine, ObjStackSlice*);

typedef struct CallFrame {
    ObjClosure* closure;
    uint8_t* ip;
    size_t stackEntryIndex;
} CallFrame;

typedef struct StackSlice {
    ValueCell elements[SLICE_MAX];
} StackSlice;

typedef struct ObjRoutine {
    Obj obj;

    vm_mutex lock;
    bool locked;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    StackSlice** stackSlices;
    size_t stackSliceCapacity;
    size_t sliceCount;

    size_t stackTopIndex;

    StackSlice stk;
    DynamicObjArray additionalSlicesArray;
    AddSliceFn addSlice;

    DynamicObjArray runtimeErrors;

    ObjClosure* entryFunction;
    Value entryArg;
    Value result;
    Value error;

    ObjUpvalue* openUpvalues;

    volatile ExecState state;
    bool traceExecution;
} ObjRoutine;

#endif // !cyarg_objroutine_h
