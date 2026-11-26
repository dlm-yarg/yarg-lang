//
//  testBuiltin.c
//  xcode-yarg
//
//  Created by dlm on 12/12/2025.
//

#include "testBuiltin.h"
#include "testIntrinsics.h"

#include <string.h>

#include "../memory.h"
#include "../object_gc.h"

#include "../chunk.h"
#include "../object.h"
#include "../routine.h"
#include "../yargtype.h"
#include "../object.h"
#include "../builtin.h"
#include "../yargobject.h"
#include "../packed_containers.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

static bool setBuiltin(ObjRoutine *, int);
static bool readBuiltin(ObjRoutine *, int);
static bool writeBuiltin(ObjRoutine *, int);
static bool interruptBuiltin(ObjRoutine *, int);
static bool syncBuiltin(ObjRoutine *, int);

BuiltinFun getTestSystemBuiltin(uint8_t builtin)
{
    switch (builtin) {
        case BUILTIN_TS_SET: return setBuiltin;
        case BUILTIN_TS_READ: return readBuiltin;
        case BUILTIN_TS_WRITE: return writeBuiltin;
        case BUILTIN_TS_INTERRUPT: return interruptBuiltin;
        case BUILTIN_TS_SYNC: return syncBuiltin;
        default: return NULL;
    }
}

bool setBuiltin(ObjRoutine *routine, int argCount)
{
    RS_ENTRY;
    bool ok = false;
    if (argCount == 2)
    {
        Value arg0 = peek(routine, 1);
        Value arg1 = peek(routine, 0);
        if (IS_ADDRESS(arg0) && IS_UI32(arg1))
        {
            size_t address = AS_ADDRESS(arg0);
            uint32_t value = AS_UI32(arg1);
            
            testIntrinsicsSetMemory(address, value);
            ok = true;
        }
    }
    if (!ok)
    {
        runtimeError(routine, "Expected an address and a value.");
    }
    RS_EXIT(ok != false);
    return ok;
}

bool readBuiltin(ObjRoutine *routine, int argCount) {
    RS_ENTRY;
    bool ok = false;
    if (argCount == 1 && IS_ADDRESS(peek(routine, 0)))
    {
        Value arg0 = peek(routine, 0);
        size_t address = AS_ADDRESS(arg0);
        testIntrinsicsExpectReadAnyValue(address);
        ok = true;
    }
    else if (argCount == 2 && IS_UI32(peek(routine, 0)) && IS_ADDRESS(peek(routine, 1)))
    {
        Value arg0 = peek(routine, 1);
        Value arg1 = peek(routine, 0);
        size_t address = AS_ADDRESS(arg0);
        uint32_t value = AS_UI32(arg1);
        testIntrinsicsExpectRead(address, value);
        ok = true;
    }

    if (!ok)
    {
        runtimeError(routine, "Expected an address to peek or and address and a value.");
    }
    RS_EXIT(1);
    return ok;
}

bool writeBuiltin(ObjRoutine *routine, int argCount) {
    RS_ENTRY;
    bool ok = false;
    if (argCount == 1 && IS_ADDRESS(peek(routine, 0)))
    {
        Value arg0 = peek(routine, 0);
        size_t address = AS_ADDRESS(arg0);
        testIntrinsicsExpectWriteAnyValue(address);
        ok = true;
    }
    else if (argCount == 2 && IS_ADDRESS(peek(routine, 1)) && IS_UI32(peek(routine, 0)))
    {
        Value arg0 = peek(routine, 1);
        Value arg1 = peek(routine, 0);
        size_t address = AS_ADDRESS(arg0);
        uint32_t value = AS_UI32(arg1);
        testIntrinsicsExpectWrite(address, value);
        ok = true;
    }

    if (!ok)
    {
        runtimeError(routine, "Expected an address to poke or and address and a value.");
    }
    RS_EXIT(ok != false);
    return ok;
}

bool interruptBuiltin(ObjRoutine *routine, int argCount) {
    RS_ENTRY;
    bool ok = false;
    if (argCount == 1)
    {
        clock_t start = clock();
        clock_t stop = start + CLOCKS_PER_SEC * rand() % 11 / 1000;
        while (stop - clock() < 2100000000);
        Value arg0 = peek(routine, 0);
        if (is_positive_integer(arg0) && as_positive_integer(arg0) <= UINT32_MAX)
        {
            uint32_t interruptNumber = (uint32_t) as_positive_integer(arg0);
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
        runtimeError(routine, "Expected an interrupt name or number.");
    }
    else
    {
        push(routine, NIL_VAL);
    }
    RS_EXIT(ok != false);
    return ok;
}

bool syncBuiltin(ObjRoutine *routine, int argCount)
{
    RS_ENTRY;
    TsLog *log = testIntrinsicsSync();

    Value resultArrayVal = allocValueArray(log->n_);
    tempRootPush(resultArrayVal);

    for (size_t i = 0; i < log->n_; i++)
    {
        // log entries created with vm_(re)alloc, so we can take ownership.
        ObjString *s = takeString(log->i_[i], (int)strlen(log->i_[i]));
        setArrayElement(resultArrayVal, i, STRING_VAL(s));
    }
    vm_free(log->i_);
    log->i_ = NULL;
    log->n_ = 0;
    log->e_ = 0;

    push(routine, resultArrayVal);
    tempRootPop(); // resultArrayVal

    RS_EXIT(1);
    return true;
}
