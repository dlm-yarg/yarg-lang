#include "builtin.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "object.h"
#include "yargobject.h"
#include "vmobject.h"

#include "value.h"
#include "native.h"
#include "routine.h"
#include "vm.h"
#include "debug.h"
#include "fs/fs.h"
#include "xip_library.h"
#include "compiler.h"
#include "channel.h"
#include "yargtype.h"
#include "sync_group.h"
#include "pack.h"
#include "packed_containers.h"

#ifdef CYARG_FEATURE_TEST_SYSTEM
#include "test-system/testSystem.h"
#include "test-system/testBuiltin.h"
#endif

// read from the filesystem, into memory.
// Long term, to be replaced with native yarg, and less
// assumptions about where filesystems live.
// reads:
//  - yarg binary files (returned as uint8[])
//  - text files (returned as string)
// Both are suitable input to load().
static bool readYargSourceBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }
    if (!IS_STRING(peek(routine, 0))) {
        runtimeError(routine, "Argument to read_yarg_source must be string.");
        return false;
    }

    const char* filename = AS_CSTRING(peek(routine, 0));
    char const *dotOn = strrchr(filename, '.');
    if (dotOn != 0 && strcmp(dotOn, ".yb") == 0) {
        size_t file_size = fileSize(filename);

        Value byteBuffer = allocByteArray(file_size);
        push(routine, byteBuffer);
        tempRootPop();

        readFileIntoBuffer(filename, AS_PACKEDCONTAINER(byteBuffer)->loc, file_size);
    }
    else {
        // assume a text file
        char* source = readFile(filename);
        if (source == NULL) {
            runtimeError(routine, "Could not read source file '%s'.", filename);
            return false;
        }

        ObjString* sourceString = copyString(source, (int)strlen(source));
        vm_free(source);

        push(routine, STRING_VAL(sourceString));
        tempRootPop();
    }
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}


// read from the ROM image, into memory.
// Long term, to be replaced with native yarg, and less
// assumptions about where filesystems live.
// reads:
//  - yarg binary files (returned as uint8[])
//  - text files (returned as string)
// Both are suitable input to load().
static bool readYargROMSourceBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!is_positive_integer(peek(routine, 0))) {
        runtimeError(routine, "First argument to read_yarg_source must be positive integer.");
        return false;
    }
    if (!is_positive_integer(peek(routine, 1))) {
        runtimeError(routine, "Second argument to read_yarg_source must be positive integer.");
        return false;
    }

    size_t romFileIndex = as_positive_integer(peek(routine, 1));
    size_t format_requested = as_positive_integer(peek(routine, 0));

    if (romFileIndex > UINT16_MAX) {
        runtimeError(routine, "ROM file index %zu out of range.", romFileIndex);
        return false;
    }

    size_t length;
    const uint8_t* data;
    if (!xipLibraryReadNode(romFileIndex, &data, &length)) {
        runtimeError(routine, "Failed to read ROM node %zu.", romFileIndex);
        return false;
    }

    if (format_requested == 1) {
        Value byteArrayValue = createByteArrayAt((uint8_t*)data, length);
        push(routine, byteArrayValue);
        tempRootPop();
    }
    else if (format_requested == 2) {
        const char* string = (const char*) data;
        ObjString* sourceString = copyString(string, (int)length);
        push(routine, STRING_VAL(sourceString));
    }
    else {
        runtimeError(routine, "Unsupported format requested: %d.", format_requested);
        return false;
    }
    TR_EXIT(0);
    RS_EXIT(1);
   return true;
}

static bool compileBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }
    if (!IS_STRING(peek(routine, 0))) {
        runtimeError(routine, "Argument to compile must be string.");
        return false;
    }

    Value compileResult = compile(routine);
    Value compileError = structField(compileResult, 0);
    Value compileFunction = structField(compileResult, 1);

    if (IS_NIL(compileError)) {
        ObjClosure* closure = newClosure(AS_FUNCTION(compileFunction));
        setStructField(compileResult, 1, CLOSURE_VAL(closure));
        tempRootPop();
    }

    tempRootPop(); // compileFunction
    tempRootPop(); // compileError

    TR_EXIT(0);
    RS_EXIT(1); // compileResult
    return true;
}

static Value resultPair(Value error, Value result) {
    TR_ENTRY;
    Value resultValue = allocValueStructPair("errors", "result");
    setStructField(resultValue, 0, error);
    setStructField(resultValue, 1, result);
    return resultValue;
    TR_EXIT(1);
}

bool loadBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    Value arg = peek(routine, 0);
    
    if (isByteArray(arg)) {
        ObjPackedContainer* container = AS_PACKEDCONTAINER(arg);
        const ObjConcreteYargTypeArray* arrayType = (const ObjConcreteYargTypeArray*) arg.type;
        uint8_t* buffer = (uint8_t*)container->loc;
        ObjFunction* function = loadPackageFromBuffer(routine, buffer, arrayType->cardinality);
        Value functionVal = function ? FUNCTION_VAL(function) : NIL_VAL;
        Value loadResult = resultPair(NIL_VAL, functionVal);
        push(routine, loadResult);
        assert(tempRootPop().as.obj == loadResult.as.obj);
        if (function != 0) {
            popNBehindTop(routine, 1); // function
        }

        setStructField(loadResult, 1, functionVal);
    } else if (IS_STRING(arg)) {
        Value compileResult = compile(routine);
        Value compileError = structField(compileResult, 0);
        Value compileFunction = structField(compileResult, 1);

        Value loadResult = resultPair(compileError, NIL_VAL);
        push(routine, loadResult);
        tempRootPop(); // loadResult
        if (IS_NIL(compileError)) {
            setStructField(loadResult, 1, compileFunction);
        }

        if (IS_NIL(compileError)) {
            Value compileFunction = structField(compileResult, 1);
            setStructField(loadResult, 1, compileFunction);
            tempRootPop(); // compileFunction
        }
        popNBehindTop(routine, 1); // compileResult
        tempRootPop(); // compileFunction
        tempRootPop(); // compileError

    } else {
        runtimeError(routine, "Argument to load must be a byte array or a string.");
        TR_EXIT(0);
        RS_EXIT(0);
        return false;
    }

    Value loadResult = peek(routine, 0);
    Value functionVal = structField(loadResult, 1);

    if (!IS_NIL(functionVal)) {
        ObjClosure* closure = newClosure(AS_FUNCTION(functionVal));
        tempRootPopBelow(1); // functionVal
        setStructField(loadResult, 1, CLOSURE_VAL(closure));
        tempRootPop(); // closure
    } else {
        tempRootPop(); // functionVal(nil_val)
    }

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool makeChannelBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount >= 2) {
        runtimeError(routine, "Expected 0 or 1 arguments but got %d.", argCount);
        return false;
    }
    Value valCapacity;
    if (argCount == 0) {
        valCapacity = UI32_VAL(1);
    } else {
        Value arg1 = peek(routine, 0);
        if (!is_positive_integer(arg1)) {
            runtimeError(routine, "Expected a positive integer");
            return false;
        }
        valCapacity = arg1;
    }

    size_t capacity = as_positive_integer(valCapacity);

    ObjChannelContainer* channel = newChannel(routine, capacity);

    if (channel == NULL) {
        return false;
    } else {
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    }
}

static bool sendChannelBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = peek(routine, 1);
    Value dataVal = peek(routine, 0);

    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannelContainer* channel = AS_CHANNEL(channelVal);
    sendChannel(channel, dataVal);
    push(routine, NIL_VAL);

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool shareChannelBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = peek(routine, 1);
    Value dataVal = peek(routine, 0);
    
    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannelContainer* channel = AS_CHANNEL(channelVal);
    bool overflow = shareChannel(channel, dataVal);
    push(routine, BOOL_VAL(overflow));

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool cpeekBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = peek(routine, 0);

    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannelContainer* channel = AS_CHANNEL(channelVal);
    push(routine, peekChannel(channel));

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool makeSyncGroupBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }
    Value items = peek(routine, 0);
    if (!IS_UNIFORMARRAY(items)) {
        runtimeError(routine, "Expected an array.");
        return false;
    }
    ObjConcreteYargTypeArray* t = (ObjConcreteYargTypeArray*)items.type;
    if (t->element_type == NULL) {
        for (size_t i = 0; i < t->cardinality; i++) {
            Value element = arrayElement(items, i);
            if (!IS_CHANNEL(element)) {
                runtimeError(routine, "Array must contain only channel items.");
                return false;
            }
            tempRootPop();
        }
    } else if (t->element_type->yt != TypeChannel) {
        runtimeError(routine, "Array must contain only channel items.");
        return false;
    }

    ObjSyncGroup* group = newSyncGroup(routine, items);

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool makeRoutineBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(peek(routine, 0))) {
        runtimeError(routine, "Argument to make_routine must be a function.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(peek(routine, 0));
    ObjRoutine* newR = newRoutine();

    if (bindEntryFn(newR, closure)) {
        push(routine, ROUTINE_VAL(newR));
        tempRootPop(); // newR
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    }
    else {
        runtimeError(routine, "Function must take 0 or 1 arguments.");
        return false;
    }
}

static bool resumeBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount < 1 || argCount > 2) {
        runtimeError(routine, "Expected one or two arguments to resume.");
        return false;
    }

    Value argVal = NIL_VAL;
    Value routineVal = NIL_VAL;
    if (argCount == 2) {
        argVal = peek(routine, 0);
        routineVal = peek(routine, 1);
    } else if (argCount == 1) {
        routineVal = peek(routine, 0);
    }

    if (!IS_ROUTINE(routineVal)) {
        runtimeError(routine, "Argument to resume must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(routineVal);

    if (!routineSuspendedOrUnbound(target)) {
        return false;
    }

    Value v;
    bool ok = resumeRoutine(routine, target, argCount == 2 ? 1 : 0, argVal, &v);
    push(routine, v);
    tempRootPop();
    
    TR_EXIT(0);
    RS_EXIT(1);
    return ok;
}

static bool startBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount < 1 || argCount > 2) {
        runtimeError(routine, "Expected one or two arguments to start.");
        return false;
    }

    Value argVal = NIL_VAL;
    Value targetRoutineVal = NIL_VAL;
    if (argCount == 2) {
        argVal = peek(routine, 0);
        targetRoutineVal = peek(routine, 1);
    } else if (argCount == 1) {
        targetRoutineVal = peek(routine, 0);
    }

    if (!IS_ROUTINE(targetRoutineVal)) {
        runtimeError(routine, "Argument to start must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(targetRoutineVal);

    bool ok = startRoutine(routine, target, argCount == 2 ? 1 : 0, argVal);
    push(routine, NIL_VAL);

    TR_EXIT(0);
    RS_EXIT(1);
    return ok;
}

static bool receiveBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments, got %d.", argCount);
        return false;
    }

    Value targetVal = peek(routine, 0);

    if (!IS_CHANNEL(targetVal) && !IS_ROUTINE(targetVal) && !IS_SYNCGROUP(targetVal)) {
        runtimeError(routine, "Argument must be a channel, a routine or a sync group.");
        return false;
    }

    bool ok;
    if (IS_CHANNEL(targetVal)) {
        push(routine, receiveChannel(AS_CHANNEL(targetVal)));
        ok = true;
    }
    else if (IS_ROUTINE(targetVal)) {
        Value v;
        ok = receiveFromRoutine(AS_ROUTINE(targetVal), &v);
        if (ok) {
            push(routine, v);
            tempRootPop();
        }
    }
    else if (IS_SYNCGROUP(targetVal)) {
        push(routine, receiveSyncGroup(AS_SYNCGROUP(targetVal)));
        ok = true;
    }
    else {
        push(routine, NIL_VAL);
        ok = true;
    }
    TR_EXIT(0);
    RS_EXIT(ok != false);
    return ok;
}

static bool peekBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;

    Value address = peek(routine, 0);

    if (!(isAddressValue(address) || isUint32Pointer(address))) {
        runtimeError(routine, "Expected an address or pointer.");
        return false;
    }

    uintptr_t nominal_address = 0;
    if (IS_POINTER(address)) {
        nominal_address = (uintptr_t) AS_PACKEDCONTAINER(address)->loc;
    } else {
        nominal_address = AS_ADDRESS(address);
    }

#if defined(CYARG_SELF_HOSTED)
    volatile uint32_t* reg = (volatile uint32_t*) nominal_address;
    uint32_t res = *reg;
    push(routine, UI32_VAL(res));
#elif defined(CYARG_OS_HOSTED)
#ifdef CYARG_FEATURE_TEST_SYSTEM
    push(routine, SIZE_T_UI_VAL(tsRead(nominal_address)));
#else
         push(routine, SIZE_T_UI_VAL(0));
#endif
    printf("peek(%p) -> %x\n", (void*)nominal_address, AS_UI32(peek(routine, 0)));
#endif
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool lenBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument, but got %d.", argCount);
        return false;
    }

    Value res = op_len(routine);
    TR_EXIT(0);
    RS_EXIT(1);
    if (!IS_NIL(res)) {
        push(routine, res);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else {
        TR_EXIT(0);
        RS_EXIT(0);
        return false;
    }
}

static bool pinBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument, but got %d.", argCount);
        return false;
    }

    Value arg = peek(routine, 0);
    if (IS_ROUTINE(arg)) {
        ObjRoutine* isrRoutine = AS_ROUTINE(arg);
        if (routineArity(isrRoutine) != 0) {
            return false;
        }

        uintptr_t addr;
        if (pinRoutine(isrRoutine, &addr)) {
            push(routine, ADDRESS_VAL(addr));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        } else {
            runtimeError(routine, "No more pinned routines available.");
            return false;
        }
    } else if (IS_UNIFORMARRAY(arg)) {
        push(routine, pinPackedValueContainer(arg));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_ADDRESS(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_POINTER(arg)) {
        push(routine, pinPackedValueContainer(arg));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else {
        runtimeError(routine, "Expected a routine, uniform array, pointer or address.");
        return false;
    }
}

Value createPointerAndTarget(ObjRoutine* routine, const ObjConcreteYargType* targetType) {
    RS_ENTRY;
    TR_ENTRY;

    const ObjConcreteYargType* pointerType = newYargPointerType(targetType);

    Value targetVal = newPackedValueContainer(targetType);
    ObjPackedContainer* targetContainer = AS_PACKEDCONTAINER(targetVal);
    tempObjRootPush(&targetContainer->obj); // todo - this shouldn’t be needed but targetContainer is GCd if not something to do with TYPED_VAL in newPackedValueContainer?

    Value pointerVal = createPackedContainerFor(targetContainer, 0, pointerType);
    push(routine, pointerVal);
    tempRootPop(); // targetContainer todo
    tempRootPop(); // targetVal
    tempRootPop(); // pointerVal
    tempRootPop(); // pointerType
    TR_EXIT(0);
    RS_EXIT(1);
    return pointerVal;
}

static bool new_Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;

    Value typeToCreate = NIL_VAL;
    if (argCount == 0) {
        typeToCreate = NIL_VAL;
    } else if (argCount == 1 && IS_YARGTYPE(peek(routine, 0))) {
        typeToCreate = peek(routine, 0);
    } else if (argCount == 1 && IS_NIL(peek(routine, 0))) {
        typeToCreate = NIL_VAL;
    } else {
        runtimeError(routine, "Expected a type to create.");
        return false;
    }

    if (IS_NIL(typeToCreate)) {
        // make an 'any' cell, ie a heap allocated Value.
        createPointerAndTarget(routine, NULL);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    }

    ConcreteYargType typeRequested = AS_YARGTYPE(typeToCreate)->yt;
    switch (typeRequested) {
        case TypeBool:
        case TypeDouble:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64:
        case TypeStruct: {
            push(routine, createPointerAndTarget(routine, AS_YARGTYPE(typeToCreate)));
            tempRootPop(); // defaultValue
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
        case TypeArray:
            // TODO: revisit when arrays become value types.
            push(routine, defaultValue(typeToCreate));
            tempRootPop(); // defaultValue
            TR_EXIT(0);
            RS_EXIT(1);
            return true;

        case TypeMap: {
            // TODO: revisit if maps become value types.
            if (isSupportedMapKeyType(AS_YARGTYPE(typeToCreate))) {
                ObjMap *map = newMap((ObjConcreteYargTypeMap*)AS_YARGTYPE(typeToCreate));
                push(routine, MAP_VAL(map));
                tempRootPop(); // map
                TR_EXIT(0);
                RS_EXIT(1);
                return true;
            } else {
                runtimeError(routine, "Unsupported map key type.");
                return false;
            }
        }
        case TypeInt:
        case TypeAddress:
            break; // both a bug!

        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeSyncGroup:
        case TypePointer: 
        case TypeYargType:
        case TypeImplementationObj:
            break; // unsupported (meaningless for types that can be nil)
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool uint64Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg)) {
        push(routine, UI64_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, UI64_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg)) {
        push(routine, UI64_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg)) {
        push(routine, UI64_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg)) {
        push(routine, UI64_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg)) {
        push(routine, UI64_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0) {
        push(routine, UI64_VAL(AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, 0, UINT64_MAX) == INT_WITHIN)
        {
            push(routine, UI64_VAL(int_to_u64(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        uint64_t i = strtoull(AS_CSTRING(arg), &end, 10);
        if (*end == 0)
        {
            push(routine, UI64_VAL(i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool int64Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg)) {
        push(routine, I64_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg)) {
        push(routine, I64_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg)) {
        push(routine, I64_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, I64_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg)) {
        push(routine, I64_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg)) {
        push(routine, I64_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT64_MAX) {
        push(routine, I64_VAL(AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, INT64_MIN, INT64_MAX) == INT_WITHIN)
        {
            push(routine, I64_VAL(int_to_i64(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        int64_t i = strtoll(AS_CSTRING(arg), &end, 10);
        if (*end == 0)
        {
            push(routine, I64_VAL(i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool uint32Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg) && AS_I8(arg) >= 0) {
        push(routine, UI32_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= 0) {
        push(routine, UI32_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= 0) {
        push(routine, UI32_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0 && AS_I64(arg) < UINT32_MAX) {
        push(routine, UI32_VAL((uint32_t)AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, UI32_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg)) {
        push(routine, UI32_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= UINT32_MAX) {
        push(routine, UI32_VAL((uint32_t)AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, 0, UINT32_MAX) == INT_WITHIN)
        {
            push(routine, UI32_VAL(int_to_u32(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        uint64_t i = strtoull(AS_CSTRING(arg), &end, 10);
        if (*end == 0 && i <= UINT32_MAX)
        {
            push(routine, UI32_VAL((uint32_t) i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(1);
    return false;
}

static bool int32Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg)) {
        push(routine, I32_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg)) {
        push(routine, I32_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= INT32_MIN && AS_I64(arg) <= INT32_MAX) {
        push(routine, I32_VAL((int32_t)AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, I32_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg)) {
        push(routine, I32_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= INT32_MAX) {
        push(routine, I32_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT32_MAX) {
        push(routine, I32_VAL((int32_t)AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, INT32_MIN, INT32_MAX) == INT_WITHIN)
        {
            push(routine, I32_VAL(int_to_i32(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        int64_t i = strtoll(AS_CSTRING(arg), &end, 10);
        if (*end == 0 && i >= INT32_MIN && i <= INT32_MAX)
        {
            push(routine, I32_VAL((int32_t) i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool uint16Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg) && AS_I8(arg) >= 0) {
        push(routine, UI16_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= 0) {
        push(routine, UI16_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= 0 && AS_I32(arg) <= UINT16_MAX) {
        push(routine, UI16_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0 && AS_I64(arg) <= UINT16_MAX) {
        push(routine, UI16_VAL(AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, UI16_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= UINT16_MAX) {
        push(routine, UI16_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= UINT16_MAX) {
        push(routine, UI16_VAL(AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, 0, UINT16_MAX) == INT_WITHIN)
        {
            push(routine, UI16_VAL((uint16_t) int_to_u32(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        uint64_t i = strtoull(AS_CSTRING(arg), &end, 10);
        if (*end == 0 && i <= UINT16_MAX)
        {
            push(routine, UI16_VAL((uint16_t) i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool int16Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg)) {
        push(routine, I16_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= INT16_MIN && AS_I32(arg) <= INT16_MAX) {
        push(routine, I16_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= INT16_MIN && AS_I64(arg) <= INT16_MAX) {
        push(routine, I16_VAL(AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, I16_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg) && AS_UI16(arg) <= INT16_MAX) {
        push(routine, I16_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= INT16_MAX) {
        push(routine, I16_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT16_MAX) {
        push(routine, I16_VAL(AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, INT16_MIN, INT16_MAX) == INT_WITHIN)
        {
            push(routine, I16_VAL((int16_t) int_to_i32(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        int64_t i = strtoll(AS_CSTRING(arg), &end, 10);
        if (*end == 0 && i >= INT16_MIN && i <= INT16_MAX)
        {
            push(routine, I16_VAL((int16_t) i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool uint8Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg) && AS_I8(arg) >= 0) {
        push(routine, UI8_VAL(AS_I8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= 0 && AS_I16(arg) <= UINT8_MAX) {
        push(routine, UI8_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= 0 && AS_I32(arg) <= UINT8_MAX) {
        push(routine, UI8_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0 && AS_I64(arg) <= UINT8_MAX) {
        push(routine, UI8_VAL(AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= UINT8_MAX) {
        push(routine, UI8_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg) && AS_UI16(arg) <= UINT8_MAX) {
        push(routine, UI8_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= UINT8_MAX) {
        push(routine, UI8_VAL(AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, 0, UINT8_MAX) == INT_WITHIN)
        {
            push(routine, UI8_VAL((uint8_t) int_to_u32(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        uint64_t i = strtoull(AS_CSTRING(arg), &end, 10);
        if (*end == 0 && i <= UINT8_MAX)
        {
            push(routine, UI8_VAL((uint8_t) i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool int8Builtin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    if (IS_I8(arg)) {
        push(routine, arg);
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= INT8_MIN && AS_I16(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_I16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= INT8_MIN && AS_I32(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_I32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= INT8_MIN && AS_I64(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_I64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI8(arg) && AS_UI8(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_UI8(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI16(arg) && AS_UI16(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_UI16(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_UI32(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT8_MAX) {
        push(routine, I8_VAL(AS_UI64(arg)));
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *i = AS_INT(arg);
        if (int_is_range(i, INT8_MIN, INT8_MAX) == INT_WITHIN)
        {
            push(routine, I8_VAL((int8_t) int_to_i32(i)));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    } else if (IS_STRING(arg)) {
        char *end;
        int64_t i = strtoll(AS_CSTRING(arg), &end, 10);
        if (*end == 0 && i >= INT8_MIN && i <= INT8_MAX)
        {
            push(routine, I8_VAL((int8_t) i));
            TR_EXIT(0);
            RS_EXIT(1);
            return true;
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

static bool intBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    int64_t i;
    size_t s;
    if (IS_I8(arg)) { // todo: optimise with switch
        i = AS_I8(arg);
        s = 1;
    } else if (IS_I16(arg)) {
        i = AS_I16(arg);
        s = 1;
    } else if (IS_I32(arg)) {
        i = AS_I32(arg);
        s = 2;
    } else if (IS_I64(arg)) {
        i = AS_I64(arg);
        s = 4;
    } else if (IS_UI8(arg)) {
        i = AS_UI8(arg);
        s = 1;
    } else if (IS_UI16(arg)) {
        i = AS_UI16(arg);
        s = 1;
    } else if (IS_UI32(arg)) {
        i = AS_UI32(arg);
        s = 2;
    } else if (IS_UI64(arg)) {
        uint64_t u = AS_UI64(arg);
        Value v;
        allocateIntValue(&v, 4);
        int_set_u(u, AS_INT(v));
        push(routine, v);
        tempRootPop();
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_STRING(arg)) {
        char *s = AS_CSTRING(arg);
        int il = INT_DIGITS_FOR_S(strlen(s));
        Value v;
        allocateIntValue(&v, il);
        int_set_s(s, AS_INT(v));
        push(routine, v);
        tempRootPop();
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else if (IS_INT(arg)) {
        Int *from = AS_INT(arg);
        int il = from->d_;
        Value v;
        allocateIntValue(&v, il);
        int_set_t(from, AS_INT(v));
        push(routine, v);
        tempRootPop();
        TR_EXIT(0);
        RS_EXIT(1);
        return true;
    } else {
        TR_EXIT(0);
        RS_EXIT(0);
        return false;
    }

    Value v;
    allocateIntValue(&v, s);
    int_set_i(i, AS_INT(v)); // todo: optimise as only IS_I64 has obj_
    push(routine, v);
    tempRootPop();
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool floatBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    double f;
    if (IS_I8(arg)) {
        f = AS_I8(arg);
    } else if (IS_I16(arg)) {
        f = AS_I16(arg);
    } else if (IS_I32(arg)) {
        f = AS_I32(arg);
    } else if (IS_I64(arg)) {
        f = AS_I64(arg);
    } else if (IS_UI8(arg)) {
        f = AS_UI8(arg);
    } else if (IS_UI16(arg)) {
        f = AS_UI16(arg);
    } else if (IS_UI32(arg)) {
        f = AS_UI32(arg);
    } else if (IS_UI64(arg)) {
        f = AS_UI64(arg);
    } else if (IS_STRING(arg))  {
        char *end;
        f = strtod(AS_CSTRING(arg), &end);
        if (*end != 0)
        {
            TR_EXIT(0);
            RS_EXIT(0);
            return false;
        }
    } else if (IS_INT(arg)) {
        char sb[INT_STRLEN_FOR_INT254];
        Int *i = AS_INT(arg);
        char const *s = int_to_s(i, sb, INT_STRLEN_FOR_INT254);
        char *end;
        f = strtod(s, &end);
        if (*end != 0)
        {
            TR_EXIT(0);
            RS_EXIT(0);
            return false;
        }
    } else if (IS_DOUBLE(arg)) {
        f = AS_DOUBLE(arg);
    } else {
        TR_EXIT(0);
        RS_EXIT(0);
        return false;
    }
    push(routine, DOUBLE_VAL(f));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

static bool stringBuiltin(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    Value arg = peek(routine, 0);
    ObjString* representation = valueToString(arg);
    push(routine, STRING_VAL(representation));
    tempRootPop(); // representation
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

ObjBuiltin* newBuiltin(BuiltinFun function) {
    ObjBuiltin* builtin = ALLOCATE_OBJ(ObjBuiltin, OBJ_BUILTIN);
    builtin->function = function;
    return builtin;
}

BuiltinFun getBuiltinFunction(uint8_t builtin) {
    switch (builtin) {
        case BUILTIN_PEEK: return peekBuiltin;
        case BUILTIN_READ_YARG_SOURCE: return readYargSourceBuiltin;
        case BUILTIN_READ_XIP_FILE: return readYargROMSourceBuiltin;
        case BUILTIN_COMPILE: return compileBuiltin;
        case BUILTIN_MAKE_ROUTINE: return makeRoutineBuiltin;
        case BUILTIN_RESUME: return resumeBuiltin;
        case BUILTIN_START: return startBuiltin;
        case BUILTIN_MAKE_CHANNEL: return makeChannelBuiltin;
        case BUILTIN_SEND: return sendChannelBuiltin;
        case BUILTIN_RECEIVE: return receiveBuiltin;
        case BUILTIN_SHARE: return shareChannelBuiltin;
        case BUILTIN_CPEEK: return cpeekBuiltin;
        case BUILTIN_MAKE_SYNCGROUP: return makeSyncGroupBuiltin;
        case BUILTIN_LEN: return lenBuiltin;
        case BUILTIN_PIN: return pinBuiltin;
        case BUILTIN_NEW: return new_Builtin;
        case BUILTIN_INT8: return int8Builtin;
        case BUILTIN_INT16: return int16Builtin;
        case BUILTIN_UINT16: return uint16Builtin;
        case BUILTIN_UINT8: return uint8Builtin;
        case BUILTIN_INT32: return int32Builtin;
        case BUILTIN_UINT32: return uint32Builtin;
        case BUILTIN_INT64: return int64Builtin;
        case BUILTIN_UINT64: return uint64Builtin;
        case BUILTIN_INT: return intBuiltin;
        case BUILTIN_MFLOAT64: return floatBuiltin;
        case BUILTIN_STRING: return stringBuiltin;
        case BUILTIN_LOAD: return loadBuiltin;
#ifndef CYARG_FEATURE_TEST_SYSTEM
        default: return NULL;
#else
        default: return getTestSystemBuiltin(builtin);
#endif
    }
}

ObjString* builtinToString(const ObjBuiltin* builtin) {
    if (builtin->function == &stringBuiltin) {
        return copyString("string", 6);
    }
    else if (builtin->function == &int8Builtin) {
        return copyString("int8", 4);
    }
    else if (builtin->function == &int16Builtin) {
        return copyString("int16", 5);
    }
    else if (builtin->function == &uint16Builtin) {
        return copyString("uint16", 6);
    }
    else if (builtin->function == &uint8Builtin) {
        return copyString("uint8", 5);
    }
    else if (builtin->function == &int32Builtin) {
        return copyString("int32", 5);
    }
    else if (builtin->function == &uint32Builtin) {
        return copyString("uint32", 6);
    }
    else if (builtin->function == &int64Builtin) {
        return copyString("int64", 5);
    }
    else if (builtin->function == &uint64Builtin) {
        return copyString("uint64", 6);
    }
    else if (builtin->function == &intBuiltin) {
        return copyString("int", 3);
    }
    else if (builtin->function == &floatBuiltin) {
        return copyString("mfloat64", 8);
    }
    else if (builtin->function == &peekBuiltin) {
        return copyString("peek", 4);
    }
    else if (builtin->function == &loadBuiltin) {
        return copyString("load", 4);
    }
    else if (builtin->function == &new_Builtin) {
        return copyString("new", 3);
    }
    else if (builtin->function == &lenBuiltin) {
        return copyString("len", 3);
    }
    else if (builtin->function == &readYargSourceBuiltin) {
        return copyString("read_yarg_source", 16);
    }
    else if (builtin->function == &readYargROMSourceBuiltin) {
        return copyString("vm_read_xip_file", 15);
    }
    else if (builtin->function == &compileBuiltin) {
        return copyString("compile", 7);
    }
    else if (builtin->function == &makeRoutineBuiltin) {
        return copyString("make_routine", 12);
    }
    else if (builtin->function == &resumeBuiltin) {
        return copyString("resume", 6);
    }
    else if (builtin->function == &startBuiltin) {
        return copyString("start", 5);
    }
    else if (builtin->function == &makeChannelBuiltin) {
        return copyString("make_channel", 12);
    }
    else if (builtin->function == &sendChannelBuiltin) {
        return copyString("send", 4);
    }
    else if (builtin->function == &receiveBuiltin) {
        return copyString("receive", 7);
    }
    else if (builtin->function == &shareChannelBuiltin) {
        return copyString("share", 5);
    }
    else if (builtin->function == &cpeekBuiltin) {
        return copyString("cpeek", 5);
    }
    else if (builtin->function == &makeSyncGroupBuiltin) {
        return copyString("make_syncgroup", 14);
    }
    return copyString("<builtin fn>", 12);
}
