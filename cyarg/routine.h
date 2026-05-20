#ifndef cyarg_routine_h
#define cyarg_routine_h

#include "object.h"

#define FRAMES_MAX 20
#define SLICE_SIZE 64

typedef struct ObjRoutine ObjRoutine;

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    size_t stackEntryIndex;
} CallFrame;

typedef enum {
    EXEC_UNBOUND,
    EXEC_RUNNING,
    EXEC_SUSPENDED,
    EXEC_CLOSED,
    EXEC_ERROR
} ExecState;

typedef struct ObjStackSlice {
    Obj obj;
    ObjPtr elements[SLICE_SIZE];
    ObjPtr sliceBelow;
} ObjStackSlice;

typedef struct ObjRoutine {
    Obj obj;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    ArrayItemCount topIndex;
    ObjPtr topSlice;

    ObjPtr entryFunction; // closure
    ObjPtr entryArg;
    ObjPtr result;

    DynamicArray *openUpvalues; // of ObjPtr, note pointer to da as wasteful to realloc ObjRoutine to extend openUpvalues

    volatile ExecState state;
    bool traceExecution;
} ObjRoutine;

void initRoutine(ObjRoutine* routine);
ObjRoutine* newRoutine(void);
void resetRoutine(ObjRoutine* routine);
bool bindEntryFn(ObjRoutine* routine, ObjClosure* closure);
void bindEntryArgs(ObjRoutine* routine, ObjPtr entryArg);
void pushEntryElements(ObjRoutine* routine);
void enterEntryFunction(ObjRoutine* routine);
ValueCell* frameSlot(ObjRoutine* routine, CallFrame* frame, size_t index);
ObjPtr nativeArgument(ObjRoutine* routine, size_t argCount, size_t argument);
size_t stackOffsetOf(CallFrame* frame, size_t frameIndex);

bool pinRoutine(ObjRoutine* routine, uintptr_t* address);
void runAndRenter(ObjRoutine* routine);

bool resumeRoutine(ObjRoutine* context, ObjRoutine* target, size_t argCount, ObjPtr argument, ObjPtr *result);
void yieldFromRoutine(ObjRoutine* routine);
void returnFromRoutine(ObjRoutine* routine, ObjPtr *result);
bool startRoutine(ObjRoutine* context, ObjRoutine* target, size_t argCount, ObjPtr argument);
bool receiveFromRoutine(ObjRoutine* routine, ObjPtr *result);

void markRoutine(ObjRoutine* routine);

void push(ObjRoutine* routine, ObjPtr value);
void pushTyped(ObjRoutine* routine, ObjPtr value, ObjPtr type);
ObjPtr pop(ObjRoutine* routine);
void popN(ObjRoutine* routine, size_t count);
void popFrame(ObjRoutine* routine, CallFrame* frame);
ObjPtr peek(ObjRoutine* routine, int distance);
ValueCell* peekCell(ObjRoutine* routine, int distance);
ValueCell* peekCellTarget(ObjRoutine* routine, int distance);

void runtimeError(ObjRoutine* routine, const char* format, ...);

#endif
