#ifndef cyarg_builtin_h
#define cyarg_builtin_h

#include "vm-result.h"

typedef struct AbstractValue *Value;
typedef struct ObjRoutine ObjRoutine;

ObjPtr getBuiltin(uint8_t builtin);

bool importBuiltinDummy(ObjRoutine* routineContext, int argCount, Value result);
InterpretResult importBuiltin(ObjRoutine* routineContext, int argCount);

#endif
