//
//  testBuiltin.c
//  xcode-yarg
//
//  Created by dlm on 12/12/2025.
//

#include "testBuiltin.h"
#include "testIntrinsics.h"

#include "../chunk.h"
#include "../object.h"
#include "../routine.h"
#include "../yargtype.h"
#include "../object.h"
#include "../memory.h"

static bool setBuiltin(ObjPtr, int, ObjPtr *);
static bool readBuiltin(ObjPtr, int, ObjPtr *);
static bool writeBuiltin(ObjPtr, int, ObjPtr *);
static bool interruptBuiltin(ObjPtr, int, ObjPtr *);
static bool syncBuiltin(ObjPtr, int, ObjPtr *);

ObjPtr getTestSystemBuiltin(uint8_t builtin)
{
    ObjPtr r = 0;
    switch (builtin) {
        case BUILTIN_TS_SET: r = newNative(setBuiltin);
        case BUILTIN_TS_READ: r = newNative(readBuiltin);
        case BUILTIN_TS_WRITE: r = newNative(writeBuiltin);
        case BUILTIN_TS_INTERRUPT: r = newNative(interruptBuiltin);
        case BUILTIN_TS_SYNC: r = newNative(syncBuiltin);
        default: ;
    }
    return r;
}

bool setBuiltin(ObjPtr r, int argCount, ObjPtr* result)
{
    bool ok = false;
    if (argCount == 2)
    {
        ObjRoutine *routineContext = AS_ROUTINE(r);
        ObjPtr arg0 = nativeArgument(routineContext, argCount, 0);
        ObjPtr arg1 = nativeArgument(routineContext, argCount, 1);
        if (arg0 != 0 && arg1 != 0)
        {
            uint32_t address = AS_UI32(arg0);
            uint32_t value = AS_UI32(arg1);
            
            testIntrinsicsSetMemory(address, value);
            ok = true;
        }
    }
    if (!ok)
    {
        ObjRoutine *routineContext = AS_ROUTINE(r);
        runtimeError(routineContext, "Expected an address and a value.");
    }
    return ok;
}

bool readBuiltin(ObjPtr r, int argCount, ObjPtr *result) {
    bool ok = false;
    ObjRoutine *routineContext = AS_ROUTINE(r);
    if (argCount >= 1)
    {
        ObjPtr arg0 = nativeArgument(routineContext, argCount, 0);
        if (argCount == 1 && arg0 != 0)
        {
            testIntrinsicsExpectReadAnyValue(address);
            ok = true;
        }
        else if (argCount == 2)
        {
            ObjPtr arg1 = nativeArgument(routineContext, argCount, 1);
            if (arg0 != 0 && arg1 != 0)
            {
                uint32_t value = AS_UI32(arg1);
                testIntrinsicsExpectRead(address, value);
                ok = true;
            }
        }
    }
    if (!ok)
    {
        runtimeError(routineContext, "Expected an address to peek or and address and a value.");
    }
    return ok;
}

bool writeBuiltin(ObjRoutine *routineContext, int argCount, ObjPtr *result) {
    bool ok = false;
    if (argCount >= 1)
    {
        ObjPtr arg0 = nativeArgument(routineContext, argCount, 0);
        uint32_t address = AS_UI32(arg0);
        if (argCount == 1 && !IS_OBJ(arg0))
        {
            testIntrinsicsExpectWriteAnyValue(address);
            ok = true;
        }
        else if (argCount == 2)
        {
            ObjPtr arg1 = nativeArgument(routineContext, argCount, 1);
            if (!IS_OBJ(arg0) && ! IS_OBJ(arg1))
            {
                uint32_t value = AS_UI32(arg1);
                testIntrinsicsExpectWrite(address, value);
                ok = true;
            }
        }
    }
    if (!ok)
    {
        runtimeError(routineContext, "Expected an address to poke or and address and a value.");
    }
    return ok;
}

bool interruptBuiltin(ObjRoutine *routineContext, int argCount, ObjPtr *result) {
    bool ok = false;
    if (argCount == 1)
    {
        ObjPtr arg0 = nativeArgument(routineContext, argCount, 0);
        if (is_positive_integer32(arg0))
        {
            uint32_t interruptNumber = as_positive_integer32(arg0);
            ok = testIntrinsicsTriggerInterrupt(interruptNumber);
        }
        else if (IS_STRING(arg0))
        {
            char *name = AS_CSTRING(arg0);
            ok = testIntrinsicsTriggerInterruptNamed(name);
        }
    }
    if (!ok)
    {
        runtimeError(routineContext, "Expected an interrupt name or number.");
    }
    return ok;
}

bool syncBuiltin(ObjRoutine *routineContext, int argCount, ObjPtr *result)
{
    TsLog *log = testIntrinsicsSync();

    Obj emptyString;
    ObjConcreteYargType *array = newYargArrayTypeFromType(OBJ_VAL(&emptyString));
    tempRootPush(OBJ_VAL(array));

    ObjConcreteYargTypeArray *arrayAsArray = (ObjConcreteYargTypeArray *)array;
    arrayAsArray->cardinality = log->n_;
    ObjPackedUniformArray* result_array = newPackedUniformArray(arrayAsArray);
    tempRootPop(); // array
    tempRootPush(OBJ_VAL(result_array));

    for (size_t i = 0; i < log->n_; i++)
    {
//        printf("%s\n", log->i_[i]); // until log gets coppied to *result
        ObjString *s = copyString(log->i_[i], (int)strlen(log->i_[i]));
        tempRootPush(OBJ_VAL(s));
        reallocate(log->i_[i], (int)strlen(log->i_[i]) + 1, 0);
        PackedValue p = arrayElement(result_array->store, i);
        assignToPackedValue(p, OBJ_VAL(s));
        tempRootPop(); // s
    }

    tempRootPop(); // array
    *result = OBJ_VAL(result_array);
    log->n_ = 0;

    return true;
}
