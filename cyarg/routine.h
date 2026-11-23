#ifndef cyarg_routine_h
#define cyarg_routine_h

#include "value.h"

#include <inttypes.h>

#define RS_ENTRY        size_t _rs_entry_ = routineStackSize(routine)
#define RS_EXIT(EX)     do { assert(routineStackSize(routine) - _rs_entry_ - (EX) == 0); } while (0)

typedef struct ObjRoutine ObjRoutine;
typedef struct ObjClosure ObjClosure;
typedef struct ObjUpvalue ObjUpvalue;
typedef struct ValueCell ValueCell;
typedef struct Value Value;
typedef struct ObjString ObjString;
typedef struct CallFrame CallFrame;

void initRoutine(ObjRoutine* routine);
ObjRoutine* newRoutine();
void gc_freeRoutine(ObjRoutine* routine);
void freeRoutine(ObjRoutine* routine);
void resetRoutine(ObjRoutine* routine, Value error);

bool bindEntryFn(ObjRoutine* routine, ObjClosure* closure);
void bindEntryArgs(ObjRoutine* routine, Value entryArg);
void pushEntryElements(ObjRoutine* routine);
void enterEntryFunction(ObjRoutine* routine);
ValueCell* frameSlot(ObjRoutine* routine, CallFrame* frame, size_t index);

bool pinRoutine(ObjRoutine* routine, uintptr_t* address);
void runAndRenter(ObjRoutine* routine);

bool resumeRoutine(ObjRoutine* context, ObjRoutine* target, size_t argCount, Value argument, Value* result);
void yieldFromRoutine(ObjRoutine* routine);
int op_return(ObjRoutine* routine, CallFrame* frame);
void op_close_upvalues(ObjRoutine* routine);
bool startRoutine(ObjRoutine* context, ObjRoutine* target, size_t argCount, Value argument);
bool receiveFromRoutine(ObjRoutine* routine, Value* result);

void push(ObjRoutine* routine, Value value);
void pushTyped(ObjRoutine* routine, Value value, Value type);
Value pop(ObjRoutine* routine);
void popN(ObjRoutine* routine, size_t count);
void popFrame(ObjRoutine* routine, CallFrame* frame);
Value peek(ObjRoutine* routine, int distance);
ValueCell* peekCell(ObjRoutine* routine, int distance);

void routineNewFrame(ObjRoutine* routine, ObjClosure* closure, int argCount);
void routineCaptureUpvalue(ObjRoutine* routine, ObjClosure* closure, int upValueIndex, bool isLocal, CallFrame* frame, uint8_t frameIndex);
CallFrame* routineCurrentFrame(ObjRoutine* routine);

void routineSetStateRunning(ObjRoutine* routine);
bool routineSuspendedOrUnbound(ObjRoutine* routine);
bool routineErrorState(ObjRoutine* routine);
int routineArity(ObjRoutine* routine);

size_t routineStackSize(ObjRoutine* routine);
bool routineStackOverflow(ObjRoutine* routine);
void runtimeError(ObjRoutine* routine, const char* format, ...);

void traceExecutionIfEnabled(ObjRoutine* routine);

ObjString* routineToString(Value routine);

#endif
