#include "object.h"

#include "object_in_rom.h"

#include "vm.h"
#include "yargtype.h"
#include "channel.h"
#include "sync_group.h"
#include "dynamic_array.h"

#include <string.h>
#include <assert.h>
#include <stdarg.h>

ObjPtr allocateObject(size_t size, ObjType type) {
    ObjPtr p = osAlloc(size);
    Obj *obj = (Obj *)osDeref(p);
    memset(obj, 0, size);

    object->type = type;
    object->isMarked = false;

    platform_critical_section_enter_blocking(&vm.heap);

    object->next = vm.objects;
    vm.objects = object;
    
    platform_critical_section_exit(&vm.heap);

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
    Obj *newObj = (Obj *)osDeref(p);
    da = (DynamicArray *)((uint8_t *) newObj + arrayOffset);
    da->arrayCapacity = numItems;
}

ObjPtr allocateIntObject(size_t numDigits) {
    numDigits += numDigits % 2; // numDigits is always even
    assert(numDigits <= 254 && numDigits >= 2);
    ObjPtr p = osAlloc(sizeof (ObjInt) + numDigits * sizeof (uint16_t));
    ObjInt *obj = (ObjInt *)osDeref(p);
    obj->obj.objType = OBJ_INT;
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


// ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
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
    ObjFunction const *f = (ObjFunction const *)osDeref(function);
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
    ObjConcreteYargTypeArray const *t = (ObjConcreteYargTypeArray const *)osDeref(type);
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
    ObjPointer const *ptr = (ObjPointer const *)osDeref(p);
    ObjAddress *addr = (ObjAddress *)osDerefAndModify(ptr->destination);
    addr->a += offset;
}

inline bool isObjOfOneType(ObjPtr obj, size_t n, ...) {
    va_list a_list;
    va_start(a_list, n);

    Obj const *o = osDeref(obj);
    if (o == 0) return false;

    for (size_t i = 0; i < n; i++) {
        if (va_arg(a_list, ObjType) == o->objType) return true;
    }
    return false;
}

bool isAddressValue(ObjPtr p) {
    Obj const *obj = osDeref(p);
    if (obj->objType == OBJ_INT) {
        ObjInt *i = (ObjInt *)osDerefAndModify(p);
        return i->isLiteral && int_is_range(&i->i, 0, UINTPTR_MAX) == INT_WITHIN;
    }
    return obj->objType == OBJ_ADDRESS || obj->objType == OBJ_POINTER || obj->objType == OBJ_PLACED_VALUE;
}

bool isArrayPointer(ObjPtr p) {
    Obj const *obj = osDeref(p);
    if (obj->objType == OBJ_POINTER) {
        ObjPointer const *pointer = (ObjPointer const *)obj;
        return pointer->type == OIR_NIL && IS_ARRAY(pointer->destination) || AS_YARGTYPE(pointer->type)->yt == OBJ_ARRAY;
    }
    return false;
}

bool isStructPointer(ObjPtr p) {
    if (IS_POINTER(p)) {
        ObjPointer const *pointer = AS_POINTER(p);
        return pointer->type == OIR_NIL && IS_STRUCT(pointer->destination) || AS_YARGTYPE(pointer->type)->yt == OBJ_STRUCT;
    }
    return false;
}

ObjPtr destinationObject(ObjPtr p) {
    if (IS_POINTER(p)) {
        ObjPointer const *ptr = AS_POINTER(p);
        return ptr->destination;
    }
    return OIR_NIL;
}

static ObjSize ytItemSize(ObjPtr t, ObjSize *alignment) {
    ObjSize r;
    ObjSize biggestItemAlignment = 0;

    ObjConcreteYargType const *yt = (ObjConcreteYargType const *)osDeref(t);
    switch (yt->yt) {
    case OBJ_ARRAY: {
        ObjConcreteYargTypeArray const *yt = (ObjConcreteYargTypeArray const *)osDeref(t);
        r = yt->cardinality * ytItemSize(yt->element_type, &biggestItemAlignment);
        break;
    }
    case OBJ_STRUCT: {
        // alignment is the alignment of the most aligned member
        // size is the multiple of alignment suficient to hold all aligned members
        ObjConcreteYargTypeStruct const *yt = (ObjConcreteYargTypeStruct const *)osDeref(t);
        for (int i = 0; i < yt->elements.arrayLength; i++) {
            YargTypeStructElement const *v = (YargTypeStructElement const *)daAt(&yt->elements, i);
            ObjSize itemAlignment;
            ObjSize itemSize = ytItemSize(v->type, &itemAlignment);
            biggestItemAlignment = biggestItemAlignment < itemAlignment ? itemAlignment : biggestItemAlignment;
        }
        ObjSize offset = 0;
        for (int i = 0; i < yt->elements.arrayLength; i++) {
            YargTypeStructElement const *v = (YargTypeStructElement const *)daAt(&yt->elements, i);
            ObjSize itemAlignment;
            ObjSize itemSize = ytItemSize(v->type, &itemAlignment);
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
    case OBJ_I8: case OBJ_UI8: biggestItemAlignment = r = sizeof (int8_t); break;
    case OBJ_I16: case OBJ_UI16: biggestItemAlignment = r = sizeof (int16_t); break;
    case OBJ_I32: case OBJ_UI32: biggestItemAlignment = r = sizeof (int32_t); break;
    case OBJ_I64: case OBJ_UI64: biggestItemAlignment = r = sizeof (int64_t); break;
    default: biggestItemAlignment = 0; r = 0; break;
    }

    if (alignment != 0) *alignment = biggestItemAlignment;
    return r;
}

ObjPtr placeObjectAt(ObjPtr type, ObjPtr location) {
    ObjPtr r;

    if (IS_ADDRESS(location)) {
        uintptr_t loc = ((ObjAddress const *)osDeref(location))->a;

        ObjConcreteYargType const *t = AS_YARGTYPE(type);
        switch (t->yt) {
        case OBJ_ARRAY: {
            r = osAlloc(sizeof (ObjPlacedArray));
            ObjPlacedArray *pa = (ObjPlacedArray *)osDeref(r);
            pa->type = type;
            ObjConcreteYargTypeArray *at = (ObjConcreteYargTypeArray *)t;
            ObjSize alignment;
            pa->elementSize = ytItemSize(at->element_type, &alignment);
            assert(loc % alignment == 0);
            pa->placedAddress = loc;
            break;
        }
        case OBJ_STRUCT: {

            break;
        }
        case OBJ_I8:
        case OBJ_UI8:
        case OBJ_I16:
        case OBJ_UI16:
        case OBJ_I32:
        case OBJ_UI32:
        case OBJ_I64:
        case OBJ_UI64: {
            r = osAlloc(sizeof (ObjPlacedValue));
            ObjPlacedValue *pv = (ObjPlacedValue *)osDeref(r);
            pv->type = type;
            pv->placedAddress = loc;
            break;
        }
        default:
            r = OIR_NIL;
        }
    }
    else {
        r = OIR_NIL;
    }
    return r;
}
bool structFieldIndex(ObjPtr t, ObjPtr name, ArrayItemCount *index) {
    ObjConcreteYargTypeStruct const *structType = (ObjConcreteYargTypeStruct const *)osDeref(t);
    for (int i = 0; i < structType->elements.arrayLength; i++) {
        YargTypeStructElement const *se = (YargTypeStructElement const *)daAt(&structType->elements, i);
        if (name == se->name) {
            *index = i;
            return true;
        }
    }
}

ObjPtr structField(DynamicArray *da, size_t i) {
    KeyValue const *kv = (KeyValue const *)daAt(da, i);
    return kv->value;
}

ObjPtr defaultStructValue(ObjPtr t) {
    ObjConcreteYargTypeStruct* type = (ObjConcreteYargTypeStruct*)osDeref(t);
    ArrayItemCount n = type->elements.arrayLength;

    ObjPtr p = ALLOCATE_VAR_OBJ(ObjStruct, OBJ_STRUCT, store, KeyValue, n);
    ObjStruct *newStruct = (ObjStruct *)osDeref(p);

    for (int i = 0; i < n; i++) {
        KeyValue *kv = (KeyValue *)daAt(&newStruct->store, i);
        YargTypeStructElement const *se = (YargTypeStructElement const *)daAt(&type->elements, i);
        kv->key = se->name;
        kv->value = OIR_NIL;
    }

    return p;
}

ObjPtr newStringWithEscapes(const char* chars, int length)
{
    char* heapChars = malloc(length + 1);

    char const *in = chars;
    char *out = heapChars;
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
    ObjPtr p = osStoreString(out);

    free(heapChars);
    return p;
}

ObjUpvalue* newUpvalue(ValueCell* slot, size_t stackOffset) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    NIL_VAL(&upvalue->closed.value);
    upvalue->closed.cellType = NULL;
    upvalue->contents = slot;
    upvalue->stackOffset = stackOffset;
    upvalue->uvNext = NULL;
    return upvalue;
}

static void printFunction(FILE* op, ObjFunction* function) {
    if (function->fName == NULL) {
        FPRINTMSG(op, "<script>");
        return;
    }
    FPRINTMSG(op, "<fn %s>", function->fName->chars);
}

static void printRoutine(FILE* op, ObjRoutine* routine) {
    FPRINTMSG(op, "<R%p>", routine);
}

static void printArray(FILE* op, ObjArray* array) {
    ObjConcreteYargTypeArray* arrayType = (ObjConcreteYargTypeArray*)array->store.storedType;
    printType(op, array->store.storedType);
    FPRINTMSG(op, ":[");
    for (int i = 0; i < arrayType->cardinality; i++) {
        PackedValue element = arrayElement(array->store, i);
        Value unpackedValue = unpackValue(element);
        fprintValue(op, unpackedValue);
        if (i < arrayType->cardinality - 1) {
            FPRINTMSG(op, ", ");
        }
    }
    FPRINTMSG(op, "]");
}

static void printPointer(FILE* op, ObjPointer* ptr) {
    FPRINTMSG(op, "<*");

    Obj *targetType = ptr->type->target_type == NULL ? 0 : (Obj *) ptr->type->target_type;
    fprintObject(op, targetType);
    FPRINTMSG(op, ":%p>", (void*) ptr->destination);
}

static void printStruct(FILE* op, ObjStruct* st) {
    ObjConcreteYargTypeStruct* structType = (ObjConcreteYargTypeStruct*)st->store.storedType;
    FPRINTMSG(op, "struct{|%zu:%zu|", structType->field_count, structType->storage_size);
    for (size_t i = 0; i < structType->field_count; i++) {
        PackedValue f = structField(st->store, i);
        Value logValue = unpackValue(f);
        fprintValue(op, logValue);
        FPRINTMSG(op, "; ");
    }
    FPRINTMSG(op, "}");
}

void fprintObject(FILE* op, Obj *obj) {
    if (obj == 0) {
        FPRINTMSG(op, "nil");
    } else
    switch (obj->type) {
        case OBJ_BOUND_METHOD:
            printFunction(op, ((ObjBoundMethod *)obj)->method->function);
            break;
        case OBJ_CLASS:
            FPRINTMSG(op, "%s", ((ObjClass *)obj)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(op, ((ObjClosure *)obj)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(op, ((ObjFunction *)obj));
            break;
        case OBJ_INSTANCE:
            FPRINTMSG(op, "%s instance", ((ObjInstance*)obj)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            FPRINTMSG(op, "<native fn>");
            break;
        case OBJ_ROUTINE:
            printRoutine(op, ((ObjRoutine *)obj));
            break;
        case OBJ_CHANNELCONTAINER:
            printChannel(op, ((ObjChannelContainer *)obj));
            break;
        case OBJ_SYNCGROUP:
            printSyncGroup(op, ((ObjSyncGroup *)obj));
            break;
        case OBJ_STRING:
            FPRINTMSG(op, "%s", ((ObjString*)obj)->chars);
            break;
        }
        case OBJ_UPVALUE:
            FPRINTMSG(op, "upvalue");
            break;
        case OBJ_UNOWNED_UNIFORMARRAY:
        case OBJ_PACKEDUNIFORMARRAY:
            printArray(op, ((ObjPackedArray *)obj));
            break;
        case OBJ_YARGTYPE:
        case OBJ_YARGTYPE_ARRAY:
        case OBJ_YARGTYPE_STRUCT:
        case OBJ_YARGTYPE_MAP:
            printType(op, ((ObjConcreteYargType *)obj));
            break;
        case OBJ_UNOWNED_PACKEDPOINTER:
        case OBJ_PACKEDPOINTER:
            printPointer(op, ((ObjPointer *)obj));
            break;
        case OBJ_UNOWNED_PACKEDSTRUCT:
        case OBJ_PACKEDSTRUCT:
            printStruct(op, ((ObjStruct *)obj));
            break;
        case OBJ_INT: {
            Int *i = &((ObjInt *)obj)->bigInt;
            char sb[INT_STRLEN_FOR_INT254];
            char const *s = int_to_s(i, sb, INT_STRLEN_FOR_INT254);
            FPRINTMSG(op, "%s", s);
            break;
        }
        case OBJ_MAP:
            FPRINTMSG(op, "<map ");
            FPRINTMSG(op, "(%d) ", ((ObjMap*)obj)->entries.count);
            printType(op, (ObjConcreteYargType *)(((ObjMap*)obj)->type));
            FPRINTMSG(op, " >");
            break;
        default:
            FPRINTMSG(op, "<implementation object %d>", obj->type);
            break;
    }
}

void printObject(Obj *obj) {
    fprintObject(stdout, obj);
}
