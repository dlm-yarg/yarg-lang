#include "objroutine.h"
#include "routine.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

#include "memory.h"
#include "object_gc_impl.h"
#include "object_dyn_array.h"
#include "vm.h"
#include "debug.h"
#include "yargobject.h"
#include "vmobject.h"
#include "value_cell.h"
#include "packed_containers.h"

typedef struct ObjStackSlice{
    Obj obj;
    StackSlice slice;
} ObjStackSlice;


bool addSlice(ObjRoutine* routine);

static void routineMutexEnter(ObjRoutine* routine) {
    vm_mutex_enter_blocking(&routine->lock);
}

static void routineMutexLeave(ObjRoutine* routine) {
    vm_mutex_exit(&routine->lock);
}

void initRoutine(ObjRoutine* routine) {
    vm_mutex_init(&routine->lock);

    StackSlice** initialStackSlices = GROW_ARRAY(StackSlice*, 0, 0, GROW_CAPACITY(0));

    routineMutexEnter(routine);

    routine->entryFunction = NULL;
    routine->entryArg = NIL_VAL;
    routine->sliceCount = 0;
    initDynamicObjArray(&routine->additionalSlicesArray);

    routine->stackSliceCapacity = GROW_CAPACITY(0);
    routine->stackSlices = initialStackSlices;

    routine->stackSlices[0] = &routine->stk;
    routine->sliceCount = 1;
    routine->addSlice = addSlice;

#ifdef DEBUG_TRACE_EXECUTION
    routine->traceExecution = true;
#else
    routine->traceExecution = false;
#endif

    routineMutexLeave(routine);

    resetRoutine(routine, NIL_VAL);
}

void resetRoutine(ObjRoutine* routine, Value e) {
    routineMutexEnter(routine);

    routine->error = e;
    if (e.type != NULL) {
        routine->state = EXEC_ERROR;
    } else {
        routine->state = EXEC_UNBOUND;
    }

    routine->result = NIL_VAL;
    freeDynamicObjArray(&routine->runtimeErrors);
    initDynamicObjArray(&routine->runtimeErrors);

    routine->stackTopIndex = 0;
    routine->frameCount = 0;
    routine->openUpvalues = NULL;

    routineMutexLeave(routine);
}

void gc_freeRoutine(ObjRoutine* routine)
{
    gc_free(routine->stackSlices);
    gc_freeDynamicObjArray(&routine->additionalSlicesArray);
    gc_freeDynamicObjArray(&routine->runtimeErrors);
}

void freeRoutine(ObjRoutine* routine) {
    vm_free(routine->stackSlices);
    freeDynamicObjArray(&routine->additionalSlicesArray);
    freeDynamicObjArray(&routine->runtimeErrors);
    vm_mutex_deinit(&routine->lock);
}

bool addSlice(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;

    ObjStackSlice* sliceObj = ALLOCATE_OBJ(ObjStackSlice, OBJ_STACKSLICE);
    if (sliceObj == NULL) return false;

    if (routine->stackSliceCapacity < routine->sliceCount + 1) {
        routine->stackSliceCapacity = GROW_CAPACITY(routine->stackSliceCapacity);
        routine->stackSlices = GROW_ARRAY(StackSlice*, routine->stackSlices, oldCapacity, routine->stackSliceCapacity);
    }

    if (routine->stackSlices) {
        routine->stackSlices[routine->sliceCount] = &sliceObj->slice;
        routine->sliceCount++;
        appendToDynamicObjArray(&routine->additionalSlicesArray, (Obj*)sliceObj);
    }

    tempRootPop();

    TR_EXIT(0);
    RS_EXIT(0);
    return (routine->stackSlices != NULL);
}

ObjRoutine* newRoutine() {
    ObjRoutine* routine = ALLOCATE_OBJ(ObjRoutine, OBJ_ROUTINE);
    tempObjRootPush((Obj*)routine);
    initRoutine(routine);
    tempRootPop();
    return routine;
}

void runAndRenter(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    InterpretResult execResult = run(routine);
    if (execResult == INTERPRET_OK) {
        pop(routine);
        pushEntryElements(routine);
        enterEntryFunction(routine);
    } else {
        printValue(routine->result);
        printValue(routine->error);
        for (int i = 0; i < routine->runtimeErrors.objectCount; i++) {
            printObj(routine->runtimeErrors.objects[i]);
        }
    }
    TR_EXIT(0);
    RS_EXIT(0);
}

bool pinRoutine(ObjRoutine* routine, uintptr_t* address) {
    RS_ENTRY;
    TR_ENTRY;

    assert(routine->entryFunction);
    assert(IS_NIL(routine->entryArg));
    assert(routine->entryFunction->function->arity == 0);

    if (installPinnedRoutine(routine, address)) {
        pushEntryElements(routine);
        enterEntryFunction(routine);
        routine->addSlice = NULL;
        TR_EXIT(0);
        RS_EXIT(0);
        return true;
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return false;
}

bool bindEntryFn(ObjRoutine* routine, ObjClosure* closure) {
    RS_ENTRY;
    TR_ENTRY;

    if (closure->function->arity == 0 || closure->function->arity == 1) {
        routine->entryFunction = closure;
        TR_EXIT(0);
        RS_EXIT(0);
        return true;
    }
    else {
        TR_EXIT(0);
        RS_EXIT(0);
        return false;
    }

}

void bindEntryArgs(ObjRoutine* routine, Value entryArg) {

    routine->entryArg = entryArg;
}

void pushEntryElements(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    push(routine, FUNCTION_VAL(routine->entryFunction));

    if (routine->entryFunction->function->arity == 1) {
        push(routine, routine->entryArg);
    }
    TR_EXIT(0);
    RS_EXIT(1);
}

void enterEntryFunction(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    callfn(routine, routine->entryFunction, routine->entryFunction->function->arity);
    TR_EXIT(0);
    RS_EXIT(0);
}

static Value resultPair(Value error, Value result) {
    TR_ENTRY;
    Value resultValue = allocValueStructPair("errors", "result");
    setStructField(resultValue, 0, error);
    setStructField(resultValue, 1, result);
    TR_EXIT(0);
    return resultValue;
}

bool resumeRoutine(ObjRoutine* routine, ObjRoutine* target, size_t argCount, Value argument, Value* result) {
    RS_ENTRY;
    TR_ENTRY;
    if (target->state == EXEC_UNBOUND) {
        push(target, FUNCTION_VAL(target->entryFunction));
    }

    if (argCount == 1) {
        bindEntryArgs(target, argument);
        push(target, argument);
    } else {
        assert(argCount == 0);
    }

    callfn(target, target->entryFunction, target->entryFunction->function->arity);

    InterpretResult execResult = run(target);
    if (execResult == INTERPRET_OK) {
        target->result = pop(target);
        Value resultValue = resultPair(NIL_VAL, target->result);
        *result = resultValue;
        TR_EXIT(0);
        RS_EXIT(0);
        return true;
    } else {
        Value resultValue = resultPair(target->error, NIL_VAL);
        *result = resultValue;
        TR_EXIT(0);
        RS_EXIT(0);
        return true;
    }
}

void yieldFromRoutine(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    routine->result = peek(routine, 0);
    routine->state = EXEC_SUSPENDED;
    TR_EXIT(0);
    RS_EXIT(0);
}

static void closeUpvalues(ObjRoutine* routine, size_t last) {
    RS_ENTRY;
    TR_ENTRY;
    while (routine->openUpvalues != NULL && routine->openUpvalues->stackOffset >= last) {
        ObjUpvalue* upvalue = routine->openUpvalues;
        upvalue->closed = *upvalue->contents;
        upvalue->contents = &upvalue->closed;
        routine->openUpvalues = upvalue->next;
    }
    TR_EXIT(0);
    RS_EXIT(0);
}

void op_close_upvalues(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);
    closeUpvalues(routine, routine->stackTopIndex - 1);
    routineMutexLeave(routine);
    pop(routine);
    TR_EXIT(0);
    RS_EXIT(0);
}

int op_return(ObjRoutine* routine, CallFrame* frame) {
    RS_ENTRY;
    TR_ENTRY;
    routine->result = pop(routine);
    closeUpvalues(routine, frame->stackEntryIndex);
    routine->frameCount--;
    popFrame(routine, frame);
    push(routine, routine->result);
    if (routine->frameCount == 0) {
        routine->state = EXEC_CLOSED;
    } else {
        routine->result = NIL_VAL;
    }
    TR_EXIT(0);
    RS_EXIT(0);
    return routine->frameCount;
}


bool startRoutine(ObjRoutine* routine, ObjRoutine* target, size_t argCount, Value argument) {
    RS_ENTRY;
    TR_ENTRY;

    if (target->state != EXEC_UNBOUND) {
        return false;
    }

    if (vm.core1 != NULL) {
        return false;
    }

    if (argCount == 1) {
        bindEntryArgs(target, argument);
    }

    pushEntryElements(target);

    enterEntryFunction(target);
    runOnCore1(target);
    
    TR_EXIT(0);
    RS_EXIT(0);
    return true;
}

bool receiveFromRoutine(ObjRoutine* routine, Value* result) {
    RS_ENTRY;
    TR_ENTRY;

#ifdef CYARG_PICO_BUSY_SYNC
    while (routine->state == EXEC_RUNNING) {
        tight_loop_contents();
    }
#endif    
        
    if (routine->state == EXEC_CLOSED || routine->state == EXEC_SUSPENDED) {
        Value resultValue = resultPair(NIL_VAL, routine->result);
        *result = resultValue;
        return true;
    } else if (routine->state == EXEC_ERROR) {
        Value resultValue = resultPair(routine->error, NIL_VAL);
        *result = resultValue;
        TR_EXIT(0);
        RS_EXIT(0);
        return true;
    } else {
        TR_EXIT(0);
        RS_EXIT(0);
        return false;
    }
}

ValueCell* frameSlot(ObjRoutine* routine, CallFrame* frame, size_t index) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);

    size_t stackElementIndex = frame->stackEntryIndex + index;
    ValueCell *vc =  peekCell(routine, (int) (routine->stackTopIndex - (stackElementIndex + 1)));

    routineMutexLeave(routine);
    TR_EXIT(0);
    RS_EXIT(0);
    return vc;
}

static size_t stackOffsetOf(CallFrame* frame, size_t frameIndex) {
    return frame->stackEntryIndex + frameIndex;
}

void markRoutine(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);

    size_t stackSize = routine->stackTopIndex;
    for (int i = (int)(stackSize - 1); i >= 0; i--) {
        markValueCell(peekCell(routine, i));
    }

    for (int i = 0; i < routine->frameCount; i++) {
        markObject((Obj*)routine->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = routine->openUpvalues;
        upvalue != NULL;
        upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markDynamicObjArray(&routine->additionalSlicesArray);
    markDynamicObjArray(&routine->runtimeErrors);

    markObject((Obj*)routine->entryFunction);
    markValue(routine->entryArg);
    markValue(routine->result);
    markValue(routine->error);

    routineMutexLeave(routine);
    TR_EXIT(0);
    RS_EXIT(0);
}

void routineNewFrame(ObjRoutine* routine, ObjClosure* closure, int argCount) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);
    CallFrame* frame = &routine->frames[routine->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->stackEntryIndex = routine->stackTopIndex - (argCount + 1);
    routineMutexLeave(routine);
    TR_EXIT(0);
    RS_EXIT(0);
}

void routineCaptureUpvalue(ObjRoutine* routine, ObjClosure* closure, int i, bool isLocal, CallFrame* frame, uint8_t index) {
    RS_ENTRY;
    TR_ENTRY;
    if (isLocal) {
        size_t stackOffset = stackOffsetOf(frame, index);
        ValueCell* local = frameSlot(routine, frame, index);

        ObjUpvalue* prevUpvalue = NULL;
        ObjUpvalue* upvalue = routine->openUpvalues;
        while (upvalue != NULL && upvalue->stackOffset > stackOffset) {
            prevUpvalue = upvalue;
            upvalue = upvalue->next;
        }

        if (upvalue != NULL && upvalue->stackOffset == stackOffset) {
            closure->upvalues[i] = upvalue;
        } else {
            ObjUpvalue* createdUpvalue = newUpvalue(local, stackOffset);
            createdUpvalue->next = upvalue;

            if (prevUpvalue == NULL) {
                routine->openUpvalues = createdUpvalue;
            } else {
                prevUpvalue->next = createdUpvalue;
            }

            closure->upvalues[i] = createdUpvalue;
            tempRootPop();
        }
    } else {
        closure->upvalues[i] = frame->closure->upvalues[index];
    }
    TR_EXIT(0);
    RS_EXIT(0);
}

CallFrame* routineCurrentFrame(ObjRoutine* routine) {
    return &routine->frames[routine->frameCount - 1];
}

void routineSetStateRunning(ObjRoutine* routine) {
    routine->state = EXEC_RUNNING;
}

bool routineSuspendedOrUnbound(ObjRoutine* routine) {
    return routine->state == EXEC_SUSPENDED || routine->state == EXEC_UNBOUND;
}

bool routineErrorState(ObjRoutine* routine) {
    bool isErrorState = routine->state == EXEC_ERROR;
    if (isErrorState) {
        assert(!IS_NIL(routine->error));
    }
    return isErrorState;
}

int routineArity(ObjRoutine* routine) {
    return routine->entryFunction->function->arity;
}

size_t routineStackSize(ObjRoutine* routine) {
    return routine->stackTopIndex;
}

bool routineStackOverflow(ObjRoutine* routine) {
    bool overflowing = routine->frameCount == FRAMES_MAX;
    if (overflowing) {
        runtimeError(routine, "Stack overflow.");
    }
    return overflowing;
}

void runtimeError(ObjRoutine* routine, const char* format, ...) {
    RS_ENTRY;
    TR_ENTRY;

    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);
    size_t buffer_required = vsnprintf(NULL, 0, format, args_copy);
    char* message_buffer = ALLOCATE(char, buffer_required + 1);
    int written = vsnprintf(message_buffer, buffer_required + 1, format, args);
    ObjString* errorMessage = takeString(message_buffer, written);
    appendToDynamicObjArray(&routine->runtimeErrors, (Obj*)errorMessage);
    assert(tempRootPop().as.obj == &errorMessage->obj);
    va_end(args);

    for (int i = routine->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &routine->frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int16_t line = 0;
        for (int s = 0; s < function->chunk.numLines; s++) {
            if (function->chunk.lines[s].address > instruction) break;
            line = function->chunk.lines[s].line;
        }
        // todo: add parenthesis after function name if appropriate
        const char* scriptName = function->fName ? function->fName->chars : "<script>";
        size_t buffer_required = snprintf(NULL, 0, "[line %d] in %s", line, scriptName);
        char* buffer = ALLOCATE(char, buffer_required + 1);
        int written = snprintf(buffer, buffer_required + 1, "[line %d] in %s", line, scriptName);
        ObjString *s = takeString(buffer, written);
        appendToDynamicObjArray(&routine->runtimeErrors, &s->obj);
        assert(tempRootPop().as.obj == &s->obj);
    }

    RS_EXIT(0);
    Value err = duplicateAsValueArray(&routine->runtimeErrors);
    resetRoutine(routine, err);
    assert(tempRootPop().as.obj == err.as.obj);
    TR_EXIT(0);
}

static ValueCell* slot(ObjRoutine* routine, size_t index) {
    size_t sliceIndex = index / SLICE_MAX;

    return &routine->stackSlices[sliceIndex]->elements[index % SLICE_MAX];
}

void push(ObjRoutine* routine, Value value) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);
    ValueCell* nextSlot = slot(routine, routine->stackTopIndex);

    nextSlot->value = value;
    nextSlot->cellType = NULL;
    routine->stackTopIndex++;

    if (routine->stackTopIndex / SLICE_MAX + 1 > routine->sliceCount) {
        if (routine->addSlice) {
            if (!routine->addSlice(routine)) {
                runtimeError(routine, "Value stack size exceeded.");
            }
        } else {
            runtimeError(routine, "Fixed Value stack size exceeded.");
        }
    }

    routineMutexLeave(routine);
    TR_EXIT(0);
    RS_EXIT(1);
}

void pushTyped(ObjRoutine* routine, Value value, Value type) {
    RS_ENTRY;
    TR_ENTRY;
    push(routine, value);
    ValueCell* top = peekCell(routine, 0);
    top->cellType = IS_NIL(type) ? NULL : AS_YARGTYPE(type);
    TR_EXIT(0);
    RS_EXIT(1);
}

Value pop(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;

    Value ret = peek(routine, 0);
    routineMutexEnter(routine);
    routine->stackTopIndex--;
    routineMutexLeave(routine);

    TR_EXIT(0);
    RS_EXIT(-1);
    return ret;
}

void popN(ObjRoutine* routine, size_t count) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);
    routine->stackTopIndex -= count;
    routineMutexLeave(routine);
    TR_EXIT(0);
    RS_EXIT(-count);
}

void popFrame(ObjRoutine* routine, CallFrame* frame) {
    RS_ENTRY;
    TR_ENTRY;
    routineMutexEnter(routine);

    routine->stackTopIndex = frame->stackEntryIndex;

    routineMutexLeave(routine);
    TR_EXIT(0);
    RS_EXIT(0);
}

Value peek(ObjRoutine* routine, int distance) {
    RS_ENTRY;
    TR_ENTRY;
    ValueCell* cell = peekCell(routine, distance);
    TR_EXIT(0);
    RS_EXIT(0);
    return cell->value;
}

ValueCell* peekCell(ObjRoutine* routine, int distance) {
    RS_ENTRY;
    TR_ENTRY;
    assert(routine->stackTopIndex >= (1 + distance));
    ValueCell* targetCell = slot(routine, routine->stackTopIndex - 1 - distance);

    TR_EXIT(0);
    RS_EXIT(0);
    return targetCell;
}

ValueCellTarget peekCellTarget(ObjRoutine* routine, int distance) {
    RS_ENTRY;
    TR_ENTRY;
    ValueCell* target = peekCell(routine, distance);
    ValueCellTarget result = { .value = &target->value, .cellType = target->cellType};
    TR_EXIT(0);
    RS_EXIT(0);
    return result;
}

ObjString* routineToString(Value routineVal) {
    TR_ENTRY;
    assert(!IS_NIL(routineVal));
    assert(IS_ROUTINE(routineVal));
    ObjRoutine* routine = AS_ROUTINE(routineVal);
    RS_ENTRY;
    int buffer_size = snprintf(NULL, 0, "<R%p>", routine);
    char* buffer = ALLOCATE(char, buffer_size + 1);
    if (routine == &vm.core0) {
        snprintf(buffer, buffer_size + 1, "<R%-*s>", buffer_size - 3, "core0");
    } else if (vm.core1 && routine == vm.core1) {
        snprintf(buffer, buffer_size + 1, "<R%-*s>", buffer_size - 3, "core1");
    } else {
        snprintf(buffer, buffer_size + 1, "<R%p>", routine);
    }
    ObjString* result = takeString(buffer, buffer_size);
    TR_EXIT(1);
    RS_EXIT(0);
    return result;
}

static void bufferEscape(const char* src, char* dest, size_t destSize) {
    size_t src_cursor = 0;
    size_t dest_cursor = 0;
    
    for (; src[src_cursor] != '\0' && dest_cursor < destSize - 1; src_cursor++) {
        if (src[src_cursor] == '\n') {
            dest[dest_cursor] = '\\';
            if (dest_cursor + 1 < destSize - 1) {
                dest[dest_cursor + 1] = 'n';
                dest_cursor++;
            }
        } else if (src[src_cursor] == '\t') {
            dest[dest_cursor] = '\\';
            if (dest_cursor + 1 < destSize - 1) {
                dest[dest_cursor + 1] = 't';
                dest_cursor++;
            }
        } else if (src[src_cursor] == '\\') {
            dest[dest_cursor] = '\\';
            if (dest_cursor + 1 < destSize - 1) {
                dest[dest_cursor + 1] = '\\';
                dest_cursor++;
            }
        } else {
            dest[dest_cursor] = src[src_cursor];
        }
        dest_cursor++;
    }
    dest[dest_cursor] = '\0';
}


// configure the grid used for stack trace display

// prefix + 5 cells
#define LOG_WIDTH 141

#define TYPEFIELD_WIDTH 7
#define VALUEFIELD_WIDTH 14

// maximum number of stack trace lines to display; 0 means no limit
#define MAX_STACK_TRACE_LINES 2

static void snprintValueDescription(char* buffer, size_t bufferSize, Value value) {
    TR_ENTRY;
    if (IS_STRING(value)) {
        ObjString* str = AS_STRING(value);
        char escaped[VALUEFIELD_WIDTH + 1];
        bufferEscape(str->chars, escaped, sizeof(escaped));
        char display[VALUEFIELD_WIDTH + 2 + 1];
        snprintf(display, sizeof(display), "\"%s\"", escaped);
        size_t displayLength = strlen(display);

        if (displayLength > bufferSize - 1) {
            int width = (int) bufferSize - 1 - 3; // account for null terminator, and "..."
            snprintf(buffer, bufferSize, "%*.*s...", width, width, display);
        } else {
            int width = (int) bufferSize - 1; // account for quotes and null terminator
            snprintf(buffer, bufferSize, "%*.*s", width, width, display);
        }
    } else {
        ObjString* valueStr = valueToString(value);
    
        assert(bufferSize > 4);
        int max_width = (int)(bufferSize - 1);

        if (valueStr->length > max_width) {
            int width = max_width - 3; // length of "..."
            snprintf(buffer, bufferSize, "%*.*s...", width, width, valueStr->chars);
        } else {
            snprintf(buffer, bufferSize, "%*.*s", max_width, max_width, valueStr->chars);
        }
        tempRootPop();
    }
    TR_EXIT(0);
}

static void snprintTypeDescription(char* buffer, size_t bufferSize, const ObjConcreteYargType* type) {
    TR_ENTRY;
    ObjString* typeStr = typeToString(type);

    assert(bufferSize > 4);
    int max_width = (int)(bufferSize - 1);
    if (typeStr->length > max_width && type) {
        int width = max_width - 3;
        snprintf(buffer, bufferSize, "%*.*s...", width, width, typeStr->chars);
    } else if (type) {
        snprintf(buffer, bufferSize, "%*.*s", max_width, max_width, typeStr->chars);
    } else {
        snprintf(buffer, bufferSize, "%*.*s", max_width, max_width, "any");
    }
    tempRootPop();
    TR_EXIT(0);
}

#define PREFIX_WIDTH 21
#define VALUECELL_WIDTH (TYPEFIELD_WIDTH + VALUEFIELD_WIDTH + 3)
#define MAX_STACK_LINE_VALUECELLS ((LOG_WIDTH - PREFIX_WIDTH) / VALUECELL_WIDTH)
static void traceValueStack(ObjRoutine* routine) {

    size_t stackSize = routine->stackTopIndex;
    size_t stack_lines = (stackSize + MAX_STACK_LINE_VALUECELLS - 1) / MAX_STACK_LINE_VALUECELLS;
    size_t max_lines = MAX_STACK_TRACE_LINES == 0 ? stack_lines : MAX_STACK_TRACE_LINES;

    for (size_t line = stack_lines - 1, line_cursor = 0; line_cursor < stack_lines && line_cursor < max_lines; line--, line_cursor++) {
        char prefix[PREFIX_WIDTH] = "                    ";
        if (line_cursor == 0) { // first line identifes the routine and the total stack size
            ObjString* routineStr = valueToString(ROUTINE_VAL(routine));
            snprintf(prefix, sizeof(prefix), "%s[%3zu]:", routineStr->chars, stackSize);
            tempRootPop();
        }
        // if this the last line to trace, and there are more stack elements skipped, then indicate that with an ellipsis
        if (line_cursor == max_lines - 1 && line_cursor != stack_lines - 1) {
            snprintf(&prefix[PREFIX_WIDTH - 4], 4, "...");
        }
        printf("%s", prefix);

        for (size_t cursor = 0; cursor < MAX_STACK_LINE_VALUECELLS; cursor++) {
            size_t slot =  line * MAX_STACK_LINE_VALUECELLS + cursor;
            if (slot >= stackSize) break;
            ValueCell* cell = peekCell(routine, (int)(stackSize - 1 - slot));

            char value_description[VALUEFIELD_WIDTH];
            snprintValueDescription(value_description, VALUEFIELD_WIDTH, cell->value);

            char type_description[TYPEFIELD_WIDTH];
            snprintTypeDescription(type_description, TYPEFIELD_WIDTH, cell->cellType);

            printf("[%s|%s]", value_description, type_description);
        }
        printf("\n");
    }
}

void traceExecutionIfEnabled(ObjRoutine* routine) {
    RS_ENTRY;
    TR_ENTRY;
    if (routine->traceExecution) {
        CallFrame* frame = &routine->frames[routine->frameCount - 1];

        traceValueStack(routine);
        char functionName[17];
        snprintf(functionName, sizeof(functionName), "%s:", frame->closure->function->fName ? frame->closure->function->fName->chars : "script");
        printf("    %-16s", functionName);
        disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
    }
    TR_EXIT(0);
    RS_EXIT(0);
}
