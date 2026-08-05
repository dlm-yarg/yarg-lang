#include "object.h"

#include "object_in_rom.h"

#include "vm.h"
#include "yargtype.h"
#include "channel.h"
#include "sync_group.h"
#include "dynamic_array.h"
#include "print.h"

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

ObjPtr allocateObject(size_t size, ObjType type) {
    ObjPtr p = osAlloc(size);
    Obj *obj = (Obj *)osDeref(p);
    memset(obj, 0, size);

    obj->objType = type;

#ifdef DEBUG_LOG_GC
    PRINTERR("%p allocate %zu for %d\n", obj, size, type);
#endif

    return p;
}

ObjPtr allocateVarObject(size_t size, ObjType type, size_t itemSize, size_t arrayOffset, size_t numItems) {
    ObjPtr p = osAlloc(size + numItems * itemSize);
    Obj *obj = (Obj *)osDeref(p);
    obj->objType = type;
    DynamicArray *da = (DynamicArray *)((uint8_t *) obj + arrayOffset);

    da->arrayCapacity = numItems;
    da->arrayLength = 0;
    da->arrayItemSize = itemSize;

    return p;
}

void extendVarObject(ObjPtr p, size_t size, size_t arrayOffset, size_t numItems) {
    Obj const *obj = osDeref(p);
    DynamicArray *da = (DynamicArray *)((uint8_t *) obj + arrayOffset);
    osRealloc(p, size + numItems * da->arrayItemSize);
    Obj const *newObj = osDeref(p);
    da = (DynamicArray *)((uint8_t *)newObj + arrayOffset);
    da->arrayCapacity = numItems;
}

ObjPtr allocateIntObject(size_t numDigits) {
    numDigits += numDigits % 2; // numDigits is always even
    assert(numDigits <= 254 && numDigits >= 2);
    ObjPtr p = allocateObject(sizeof (ObjInt) + numDigits * sizeof (uint16_t), OBJ_INT);
    ObjInt *obj = (ObjInt *)osDeref(p);
    obj->i.m_ = numDigits;
    return p;
}

void appendToDynamicObjArray(DynamicArray* array, ObjPtr p) { // call EXTEND_VAR_OBJ first
    assert(array->arrayCapacity > array->arrayLength);
    ObjPtr *arrayStore = (ObjPtr *)array->arrayItems;
    arrayStore[array->arrayLength++] = p;
}

ObjPtr removeLastFromDynamicObjArray(DynamicArray* array) {
    assert(array->arrayLength > 0);
    ObjPtr *arrayStore = (ObjPtr *)array->arrayItems;
    return arrayStore[--array->arrayLength];
}


// ObjBoundMethod* newBoundMethod(ObjPtr receiver, ObjClosure* method) {
ObjPtr newBoundMethod(ObjPtr receiver, ObjPtr method) {
    ObjPtr p = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    ObjBoundMethod *bound = (ObjBoundMethod *)osDeref(p);
    bound->receiver = receiver;
    bound->method = method;
    return p;
}

ObjPtr newClass(ObjPtr name) {
    ObjPtr p = ALLOCATE_VAR_OBJ(ObjClass, OBJ_CLASS, methods, ObjPtr, 4);
    ObjClass *klass = (ObjClass *)osDeref(p);
    klass->name = name;
    return p;
}

ObjPtr newClosure(ObjPtr function) {
    ObjFunction const *f = osDeref(function);
    int upvalueCount = f->upvalueCount;
    ObjPtr p = ALLOCATE_VAR_OBJ(ObjClosure, OBJ_CLOSURE, upvalues, ObjPtr, upvalueCount);
    ObjClosure* closure = (ObjClosure *)osDeref(p);
    closure->function = function;
    closure->upvalues.arrayLength = upvalueCount;
    return p;
}

ObjPtr newFunction(void) {
    ObjPtr function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    initFunction(function);
    return function;
}

void initFunction(ObjPtr function) {
    ObjFunction *f = (ObjFunction *)osDerefAndModify(function);
    initChunk(&f->chunk);
}

ObjPtr newInstance(ObjPtr klass) {
    ObjPtr p = ALLOCATE_VAR_OBJ(ObjInstance, OBJ_INSTANCE, fields, KeyValue, 4);
    ObjInstance *instance = (ObjInstance *)osDeref(p);
    instance->klass = klass;
    return p;
}

ObjPtr newNative(NativeFn function) {
    ObjPtr p = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    ObjNative *native = (ObjNative *)osDeref(p);
    native->function = function;
    return p;
}

ObjPtr newInt(int64_t value) {
    ObjPtr p = allocateIntObject(sizeof value / sizeof (uint16_t));
    Int *i = &((ObjInt *)osDeref(p))->i;
    int_set_i(value, i);
    return p;
}

ObjPtr newIntU(uint64_t value) {
    ObjPtr p = allocateIntObject(sizeof value / sizeof (uint16_t));
    Int *i = &((ObjInt *)osDeref(p))->i;
    int_set_u(value, i);
    return p;
}

void *arrayAt(DynamicArray *array, size_t index) {
    assert(index < array->arrayLength);
    return ((uint8_t *)array->arrayItems) + index * array->arrayItemSize;
}

static void initialisePackedValue(ObjPtr type, void *);

ObjPtr newArray(ObjPtr type) {
    ObjConcreteYargTypeArray const *t = osDeref(type);
    size_t c = t->cardinality;
    ObjPtr et = t->element_type;
    size_t sz = yt_sizeof_type_storage(et);

    ObjPtr p = ALLOCATE_VAR_OBJ(ObjArray, OBJ_ARRAY, elements, ObjPtr, c);
    osNoGc(p);
    ObjArray *a = (ObjArray *)osDeref(p);

    for (size_t i = 0; i < c; i++) {
        void *el = arrayAt(&a->elements, i);
        initialisePackedValue(et, el);
    }

    osGcOk(p);
    return p;
}

ObjPtr defaultArray(ObjPtr type) {
    return ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
}

ObjPtr newMap(ObjPtr type) {
    return ALLOCATE_VAR_OBJ(ObjMap, OBJ_MAP, entries, KeyValue, 0);
}

ObjPtr newPointerForHeapCell(ObjPtr type, ObjPtr location) {

    ObjPtr p = ALLOCATE_OBJ(ObjPointer, OBJ_POINTER);
    ObjPointer *ptr = (ObjPointer *)osDeref(p);
    ptr->type = type;
    ptr->destination = location;
    return p;
}

ObjPtr newPointerAtHeapCell(ObjPtr type, ObjPtr location) { // returns a new ObjPlacedValue for placed-arrays/placed-structs or ObjPointer for non-placed collections
    ObjPtr p = ALLOCATE_OBJ(ObjPointer, OBJ_POINTER);
    osNoGc(p);
    ObjPointer *ptr = (ObjPointer *)osDeref(p);
    ptr->type = type;
    ObjConcreteYargTypePointer *t = (ObjConcreteYargTypePointer *)osDeref(ptr->type);
    t->target_type = type;
    ptr->destination = location;
    osGcOk(p);
    return p;
}

void offsetPointerDestination(ObjPtr p, size_t offset) {
    ObjPointer const *ptr = osDeref(p);
    ObjAddress *addr = (ObjAddress *)osDerefAndModify(ptr->destination);
    addr->a += offset;
}

inline bool isObjOfOneType(ObjPtr obj, size_t n, ...) {
    va_list a_list;
    va_start(a_list, n);

    Obj const *o = osDeref(obj);
    if (o == 0) return false;

    for (size_t i = 0; i < n; i++) {
        if ((ObjType)va_arg(a_list, int) == o->objType) return true;
    }
    return false;
}

bool isAddressValue(ObjPtr p) {
    Obj const *obj = osDeref(p);
    if (obj->objType == OBJ_INT) {
        ObjInt *i = (ObjInt *)osDerefAndModify(p);
        return i->isLiteral && int_is_range(&i->i, 0, UINTPTR_MAX) == INT_WITHIN;
    }
    return obj->objType == OBJ_ADDRESS || obj->objType == OBJ_POINTER || obj->objType == OBJ_PLACED;
}

bool isArrayPointer(ObjPtr p) {
    Obj const *obj = osDeref(p);
    if (obj->objType == OBJ_POINTER) {
        ObjPointer const *pointer = (ObjPointer const *)obj;
        return pointer->type == OBJ_POINTER && IS_ARRAY(pointer->destination);
    }
    return false;
}

bool isStructPointer(ObjPtr p) {
    if (IS_POINTER(p)) {
        ObjPointer const *pointer = AS_POINTER(p);
        return pointer->type == OBJ_POINTER && IS_STRUCT(pointer->destination);
    }
    return false;
}

ObjPtr destinationObject(ObjPtr p) {
    if (IS_POINTER(p)) {
        ObjPointer const *ptr = AS_POINTER(p);
        return ptr->destination;
    }
    return OBJ_PTR_NIL;
}

static ObjSize ytItemSize(ObjPtr t, ObjSize *alignment) {
    ObjSize r;
    ObjSize biggestItemAlignment = 0;

    switch (t) {
    case OBJ_PTR_I8_TYPE: case OBJ_PTR_UI8_TYPE: biggestItemAlignment = r = sizeof (int8_t); break;
    case OBJ_PTR_I16_TYPE: case OBJ_PTR_UI16_TYPE: biggestItemAlignment = r = sizeof (int16_t); break;
    case OBJ_PTR_I32_TYPE: case OBJ_PTR_UI32_TYPE: biggestItemAlignment = r = sizeof (int32_t); break;
    case OBJ_PTR_I64_TYPE: case OBJ_PTR_UI64_TYPE: biggestItemAlignment = r = sizeof (int64_t); break;
    default: {
        Obj const *o = osDeref(t);
        switch (o->objType) {
        case OBJ_ARRAY: {
            // alignment is the alignment of the elements
            // size is the multiple of alignment suficient to hold all  elements
            ObjConcreteYargTypeArray const *ao = (ObjConcreteYargTypeArray const *)(o);
            r = ao->cardinality * ytItemSize(ao->element_type, &biggestItemAlignment);
            break;
        }
        case OBJ_STRUCT: {
            // alignment is the alignment of the most aligned member
            // size is the multiple of alignment suficient to hold all aligned members
            ObjConcreteYargTypeStruct const *yt = osDeref(t);
            ObjSize offset = 0;
            for (int i = 0; i < yt->elements.arrayLength; i++) {
                YargTypeStructElement const *v = (YargTypeStructElement const *)daAt(&yt->elements, i);
                ObjSize itemAlignment;
                ObjSize itemSize = ytItemSize(v->type, &itemAlignment);
                biggestItemAlignment = biggestItemAlignment < itemAlignment ? itemAlignment : biggestItemAlignment;
                ObjSize padding = offset % itemAlignment;
                if (padding > 0) padding = itemAlignment - padding;
                offset += padding;
                offset += itemSize;
            }
            ObjSize padding = offset % biggestItemAlignment;
            if (padding > 0) padding = biggestItemAlignment - padding;
            r = offset + padding;
            break;
        }
            
        default:
            biggestItemAlignment = 0; r = 0; break; // todo assert?
        }
    }
    }
    if (alignment != 0) *alignment = biggestItemAlignment;
    return r;
}

ObjPtr placeObjectAt(ObjPtr type, ObjPtr location) {
    ObjPtr r;
    if (IS_ADDRESS(location)) {
        r = allocateObject(sizeof (ObjPlaced), OBJ_PLACED);
        ObjPlaced *o = (ObjPlaced *)osDeref(r);

        o->placedObj = type;
        o->placedAddress = ((ObjAddress const *)osDeref(location))->a;
    }
    else {
        r = OBJ_PTR_NIL; // todo assert?
    }
    return r;
}

bool structFieldIndex(ObjPtr t, ObjPtr name, ArrayItemCount *index) {
    ObjConcreteYargTypeStruct const *structType = osDeref(t);
    for (int i = 0; i < structType->elements.arrayLength; i++) {
        YargTypeStructElement const *se = (YargTypeStructElement const *)daAt(&structType->elements, i);
        if (name == se->name) {
            *index = i;
            return true;
        }
    }
    return false;
}

ObjPtr structField(DynamicArray *da, size_t i) {
    KeyValue const *kv = (KeyValue const *)daAt(da, i);
    return kv->value;
}

ObjPtr defaultStructValue(ObjPtr t) {
    ObjConcreteYargTypeStruct const *type = osDeref(t);
    ArrayItemCount n = type->elements.arrayLength;

    ObjPtr p = ALLOCATE_VAR_OBJ(ObjStruct, OBJ_STRUCT, store, KeyValue, n);
    ObjStruct *newStruct = (ObjStruct *)osDeref(p);

    for (int i = 0; i < n; i++) {
        KeyValue *kv = (KeyValue *)daAt(&newStruct->store, i);
        YargTypeStructElement const *se = (YargTypeStructElement const *)daAt(&type->elements, i);
        kv->key = se->name;
        kv->value = OBJ_PTR_NIL;
    }

    return p;
}

ObjPtr newStringWithEscapes(const char* chars, int length)
{
    ObjPtr nsp = allocateObject(sizeof (ObjString) + length + 1, OBJ_STRING);
    ObjString *ns = (ObjString *)osDeref(nsp);
    osNoGc(nsp);
    char *out = ns->chars;

    char const *in = chars;
    int lengthOut = 0;
    for (int i = 0; i < length && *in != '\0'; i++)
    {
        if (*in == '\\')
        {
            in++;
            i++;
        }
        *out++ = *in++;
        lengthOut++;
    }

    *out = '\0';

    osGcOk(nsp);
    return nsp;
}

ObjPtr newUpvalue(size_t stackOffset) {
    ObjPtr p = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    ObjUpvalue *upvalue = (ObjUpvalue *)osDeref(p);
    upvalue->closed = OBJ_PTR_NIL;
    upvalue->contents = OBJ_PTR_NIL;
    upvalue->stackOffset = stackOffset;
    return p;
}

static void printFunction(FILE* op, ObjFunction const *function) {
    if (function->fName == OBJ_PTR_NIL) {
        FPRINTMSG(op, "<script>");
        return;
    }
    FPRINTMSG(op, "<fn %s>", osStringAsCString(function->fName));
}

static void printRoutine(FILE* op, ObjRoutine const *routine) {
    FPRINTMSG(op, "<R%p>", routine);
}

static void printArray(FILE* op, ObjArray const *array) {
    FPRINTMSG(op, "[");
    for (int i = 0; i < array->elements.arrayLength; i++) {
        ObjPtr p = *(ObjPtr const *)daAt(&array->elements, i);
        fprintValue(op, p);
        if (i < array->elements.arrayLength - 1) {
            FPRINTMSG(op, ", ");
        }
    }
    FPRINTMSG(op, "]");
}

static void printPointer(FILE* op, ObjPointer const *ptr) {
    FPRINTMSG(op, "<*");

    fprintObject(op, ptr->type);
    FPRINTMSG(op, ":%d>", (int)ptr->destination);
}

static void printPlaced(FILE* op, ObjPlaced const *placed) {
    FPRINTMSG(op, "<*");

    fprintObject(op, placed->placedObj);
    FPRINTMSG(op, ":%d>", (int)placed->placedAddress);
}

static void printStruct(FILE* op, ObjStruct const *st) {
    FPRINTMSG(op, "struct{|%d|", (int)st->store.arrayLength);
    for (size_t i = 0; i < st->store.arrayLength; i++) {
        KeyValue const *kv = (KeyValue const *)daAt(&st->store, i);
        fprintValue(op, kv->key);
        FPRINTMSG(op, "-");
        fprintValue(op, kv->value);
        if (i < st->store.arrayLength - 1) {
            FPRINTMSG(op, "; ");
        }
    }
    FPRINTMSG(op, "}");
}

void fprintObject(FILE* op, ObjPtr p) {
    switch (p) {
    case OBJ_PTR_ANY_TYPE:
    case OBJ_PTR_BOOL_TYPE:
    case OBJ_PTR_INT_TYPE:
    case OBJ_PTR_ADDRESS_TYPE:
    case OBJ_PTR_DOUBLE_TYPE:
    case OBJ_PTR_I8_TYPE:
    case OBJ_PTR_UI8_TYPE:
    case OBJ_PTR_I16_TYPE:
    case OBJ_PTR_UI16_TYPE:
    case OBJ_PTR_I32_TYPE:
    case OBJ_PTR_UI32_TYPE:
    case OBJ_PTR_I64_TYPE:
    case OBJ_PTR_UI64_TYPE:
    case OBJ_PTR_BOUND_METHOD_TYPE:
    case OBJ_PTR_CLASS_TYPE:
    case OBJ_PTR_CLOSURE_TYPE:
    case OBJ_PTR_FUNCTION_TYPE:
    case OBJ_PTR_INSTANCE_TYPE:
    case OBJ_PTR_NATIVE_TYPE:
    case OBJ_PTR_ROUTINE_TYPE:
    case OBJ_PTR_CHANNELCONTAINER_TYPE:
    case OBJ_PTR_STRING_TYPE:
        printType(op, p);
    default: {
        Obj const *obj = osDeref(p);
        switch (obj->objType) {
        case OBJ_BOUND_METHOD: {
            ObjClosure const *cl = osDeref(((ObjBoundMethod const *)obj)->method);
            printFunction(op, (ObjFunction const *)osDeref(cl->function));
            break;
        }
        case OBJ_CLASS:
            FPRINTMSG(op, "%s", osStringAsCString(((ObjClass const *)obj)->name));
            break;
        case OBJ_CLOSURE:
            printFunction(op, (ObjFunction const *)osDeref(((ObjClosure const *)obj)->function));
            break;
        case OBJ_FUNCTION:
            printFunction(op, ((ObjFunction const *)obj));
            break;
        case OBJ_INSTANCE: {
            ObjClass const *cl = osDeref(((ObjInstance const *)obj)->klass);
            FPRINTMSG(op, "%s instance", osStringAsCString(cl->name));
            break;
        }
        case OBJ_NATIVE:
            FPRINTMSG(op, "<native fn>");
            break;
        case OBJ_ROUTINE:
            printRoutine(op, ((ObjRoutine const *)obj));
            break;
        case OBJ_CHANNELCONTAINER:
            printChannel(op, ((ObjChannelContainer const *)obj));
            break;
        case OBJ_SYNCGROUP:
            printSyncGroup(op, p);
            break;
        case OBJ_STRING:
            FPRINTMSG(op, "%s", osStringAsCString(p));
            break;
        case OBJ_UPVALUE:
            FPRINTMSG(op, "upvalue");
            break;
        case OBJ_ARRAY:
            printArray(op, (ObjArray const *)obj);
            break;
        case OBJ_PLACED:
            printPlaced(op, (ObjPlaced const *)obj);
            break;
        case OBJ_YARGTYPE_ARRAY:
        case OBJ_YARGTYPE_STRUCT:
        case OBJ_YARGTYPE_MAP:
            printType(op, p);
            break;
        case OBJ_POINTER:
            printPointer(op, (ObjPointer const *)obj);
            break;
        case OBJ_STRUCT:
            printStruct(op, (ObjStruct const *)obj);
            break;
        case OBJ_INT: {
            Int *i = &((ObjInt *)obj)->i;
            char sb[INT_STRLEN_FOR_INT254];
            char const *s = int_to_s(i, sb, INT_STRLEN_FOR_INT254);
            FPRINTMSG(op, "%s", s);
            break;
        }
        case OBJ_MAP:
            FPRINTMSG(op, "<map ");
            FPRINTMSG(op, "(%d) ", ((ObjMap *)obj)->entries.arrayLength);
            FPRINTMSG(op, " >");
            break;
        default:
            FPRINTMSG(op, "<implementation object %d>", obj->objType);
            break;
        }
        break;
    }
    }
}

void printObject(ObjPtr p) {
    fprintObject(stdout, p);
}
