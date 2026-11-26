#include "native.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "routine.h"
#include "vm.h"
#include "fs/fs.h"
#if defined(CYARG_FEATURE_HOSTED_REPL)
#include "hosted.h"
#endif

#ifdef CYARG_PICO_SDK_TARGET
#include <hardware/irq.h>
#include <hardware/clocks.h>
#endif

#ifdef CYARG_FEATURE_TEST_SYSTEM
#include "test-system/testSystem.h"
#endif

#include "xip_library.h"

static Value nativeArgument(ObjRoutine* routine, size_t argCount, size_t argument) {
    return peek(routine, (int)argCount - 1 - (int)argument);
}

bool irq_add_shared_handlerNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;

    Value numVal = nativeArgument(routine, argCount, 0);
    Value address = nativeArgument(routine, argCount, 1);
    Value prioVal = nativeArgument(routine, argCount, 2);

    if (!IS_ADDRESS(address)) {
        runtimeError(routine, "Expected an address.");
        return false;
    }
    if (!is_positive_integer(numVal)) {
        runtimeError(routine, "Expected an a positive integer.");
        return false;
    }
    if (!is_positive_integer(prioVal)) {
        runtimeError(routine, "Expected an a positive integer for priority.");
        return false;
    }        

    size_t num = as_positive_integer(numVal);
    if (num > UINT32_MAX) {
        runtimeError(routine, "Interrupt must fit in uint32");
        return false;
    }
    
    uint32_t interrupt_num = (uint32_t) num;
    uintptr_t isrRoutine = AS_ADDRESS(address);

#if defined(CYARG_PICO_SDK_TARGET)
    size_t prio = as_positive_integer(prioVal);
    irq_add_shared_handler(interrupt_num, (irq_handler_t) isrRoutine, prio);
#elif defined(CYARG_FEATURE_TEST_SYSTEM)
    tsAddInterruptHandler(interrupt_num, (void *) isrRoutine);
#else
    #error "No IRQ handler implementation for this build."
#endif

    push(routine, NIL_VAL);
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool irq_remove_handlerNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;

    Value numVal = nativeArgument(routine, argCount, 0);
    Value address = nativeArgument(routine, argCount, 1);

    if (!IS_ADDRESS(address)) {
        runtimeError(routine, "Expected an address.");
        return false;
    }
    if (!is_positive_integer(numVal)) {
        runtimeError(routine, "Expected an a positive integer.");
        return false;
    }
    size_t num = as_positive_integer(numVal);
    if (num > UINT32_MAX) {
        runtimeError(routine, "Interrupt must fit in uint32");
        return false;
    }
    
    uint32_t interrupt_num = (uint32_t) num;
    uintptr_t isrRoutine = AS_ADDRESS(address);

#if defined(CYARG_PICO_SDK_TARGET)
    irq_remove_handler(interrupt_num, (irq_handler_t) isrRoutine);
#elif defined(CYARG_FEATURE_TEST_SYSTEM)
    tsRemoveInterruptHandler(interrupt_num, (void *) isrRoutine);
#else
    #error "No IRQ handler implementation for this build."
#endif

    removePinnedRoutine(isrRoutine);
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}


bool clockNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    push(routine, SIZE_T_UI_VAL(clock() / CLOCKS_PER_SEC));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool clock_get_hzNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    Value numVal = nativeArgument(routine, argCount, 0);

    if (!is_positive_integer(numVal)) {
        runtimeError(routine, "Argument must be a positive integer");
        return false;
    }

    size_t res = 0;

#ifdef CYARG_PICO_SDK_TARGET
    res = clock_get_hz(as_positive_integer(numVal));
#endif

    push(routine, SIZE_T_UI_VAL(res));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool stdin_getsNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        push(routine, NIL_VAL);
//        return true;
    }
    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        length--;
    }
    push(routine, STRING_VAL(copyString(buffer, (int) length)));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool stdin_eofNative(ObjRoutine* routine, int argCount) {
    push(routine, BOOL_VAL(feof(stdin)));
    return true;
}

bool stdout_putsNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value strVal = nativeArgument(routine, argCount, 0);
    if (!IS_STRING(strVal)) {
        runtimeError(routine, "Expected a string.");
        return false;
    }
#ifdef CYARG_PICO_STDLIB
    printf("%s", AS_CSTRING(strVal));
    push(routine, I32_VAL(0));
#else
    int32_t res = fputs(AS_CSTRING(strVal), stdout);
    push(routine, I32_VAL(res));
#endif
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool stderr_putsNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value strVal = nativeArgument(routine, argCount, 0);
    if (!IS_STRING(strVal)) {
        runtimeError(routine, "Expected a string.");
        return false;
    }
#ifdef CYARG_PICO_STDLIB
    printf("%s", AS_CSTRING(strVal));
    push(routine, I32_VAL(0));
#else
    int32_t res = fputs(AS_CSTRING(strVal), stderr);
    push(routine, I32_VAL(res));
#endif
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool stderr_put_newlineNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

#ifdef CYARG_PICO_STDLIB
    printf("\n");
    push(routine, I32_VAL(0));
#else
    int32_t res = fputs("\n", stderr);
    push(routine, I32_VAL(res));
#endif
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}


#if defined(CYARG_FEATURE_HOSTED_REPL)
bool host_argcNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    push(routine, I32_VAL(vmHost.argc));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool host_argnNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value indexVal = nativeArgument(routine, argCount, 0);
    if (!is_positive_integer(indexVal)) {
        runtimeError(routine, "Expected a positive integer.");
        return false;
    }
    size_t index = as_positive_integer(indexVal);
    if (index >= vmHost.argc) {
        runtimeError(routine, "Argument index out of range.");
        return false;
    }

    ObjString *s = copyString(vmHost.argv[index], (int) strlen(vmHost.argv[index]));
    push(routine, STRING_VAL(s));
    tempRootPop(); // s
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool host_exitCodeNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value codeVal = nativeArgument(routine, argCount, 0);
    if (!is_positive_integer(codeVal)) {
        runtimeError(routine, "Expected a positive integer.");
        return false;
    }
    size_t code = as_positive_integer(codeVal);
    vmHost.exitCode = (int) code;
    push(routine, NIL_VAL);
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}
#endif

bool readFileIntoBufferNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 3) {
        runtimeError(routine, "Expected 3 arguments but got %d.", argCount);
        return false;
    }

    Value pathVal = nativeArgument(routine, argCount, 0);
    Value bufferVal = nativeArgument(routine, argCount, 1);
    Value bufferSizeVal = nativeArgument(routine, argCount, 2);

    if (!is_positive_integer(bufferSizeVal)) {
        runtimeError(routine, "Expected a positive integer for the buffer size.");
        return false;
    }

    if (!IS_STRING(pathVal)) {
        runtimeError(routine, "Expected a string for the file path.");
        return false;
    }
    if (!IS_ADDRESS(bufferVal)) {
        runtimeError(routine, "Expected an address for the buffer.");
        return false;
    }

    const char* path = AS_CSTRING(pathVal);
    size_t bufferSize = as_positive_integer(bufferSizeVal);
    uint8_t* buf = (uint8_t*) AS_ADDRESS(bufferVal);

    readFileIntoBuffer(path, buf, bufferSize);

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool fileSizeNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value pathVal = nativeArgument(routine, argCount, 0);
    if (!IS_STRING(pathVal)) {
        runtimeError(routine, "Expected a string.");
        return false;
    }
    const char* path = AS_CSTRING(pathVal);

    size_t size = fileSize(path);

    push(routine, SIZE_T_UI_VAL(size));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool fileExistsNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value pathVal = nativeArgument(routine, argCount, 0);
    if (!IS_STRING(pathVal)) {
        runtimeError(routine, "Expected a string.");
        return false;
    }
    const char* path = AS_CSTRING(pathVal);

    push(routine, BOOL_VAL(fileExists(path)));
    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}

bool vm_xip_string_nodeNative(ObjRoutine* routine, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 argument but got %d.", argCount);
        return false;
    }

    Value nodeVal = nativeArgument(routine, argCount, 0);
    if (!is_positive_integer(nodeVal)) {
        runtimeError(routine, "Expected a positive integer for the node.");
        return false;
    }

    size_t node = as_positive_integer(nodeVal);
    if (node > UINT16_MAX) {
        runtimeError(routine, "Node value out of range.");
        return false;
    }
    uint16_t node16 = (uint16_t) node;

    const char* string = xipLibraryStringNode(node16);
    if (string == NULL) {
        runtimeError(routine, "String node not found.");
        return false;
    }

    Value stringVal = cstring_val(string);
    push(routine, stringVal);
    tempRootPop(); // stringVal

    TR_EXIT(0);
    RS_EXIT(1);
    return true;
}
