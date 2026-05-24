#include "value.h"

#include "object.h"
#include "dynamic_array.h"
#include "print.h"

#include <assert.h>
#include <string.h>
#include <inttypes.h>

ObjPtr copyValue(ObjPtr p) {
    return osCopy(p);
}

bool is_positive_integer32(ObjPtr p) {
    Obj const *obj = osDeref(p);
    ObjType t = obj->objType;

    if (t == OBJ_UI32 || t == OBJ_UI16 || t == OBJ_UI8) {
        return true;
    } else if (t == OBJ_UI64 && ((ObjUi64 const *)obj)->i <= UINT32_MAX) {
        return true;
    } else if (t == OBJ_I32 && ((ObjI32 const *)obj)->i >= 0) {
        return true;
    } else if (t == OBJ_I16 && ((ObjI16 const *)obj)->i >= 0) {
        return true;
    } else if (t == OBJ_I8 && ((ObjI8 const *)obj)->i >= 0) {
        return true;
    } else if (t == OBJ_I64) {
        int64_t i = ((ObjI64 const *)obj)->i;
        return i >= 0 && i <= UINT32_MAX;
    } else if (t == OBJ_INT) {
        return int_is_range(&((ObjInt const *)obj)->i, 0, UINT32_MAX) == INT_WITHIN;
    }
    return false;
}

uint32_t as_positive_integer32(ObjPtr p) {
    Obj const *obj = osDeref(p);

    switch (obj->objType) {
    case OBJ_UI32: return ((ObjUi32 const *)obj)->i;
    case OBJ_UI16: return ((ObjUi16 const *)obj)->i;
    case OBJ_UI8: return ((ObjUi8 const *)obj)->i;
    case OBJ_I32: {
        int32_t i = ((ObjI32 const *)obj)->i;
        return i >= 0 ? (uint32_t)i : 0u;
    }
    case OBJ_I16: {
        int16_t i = ((ObjI16 const *)obj)->i;
        return i >= 0 ? (uint32_t)i : 0u;
    }
    case OBJ_I8: {
        int8_t i = ((ObjI8 const *)obj)->i;
        return i >= 0 ? (uint32_t)i : 0u;
    }
    case OBJ_UI64: {
        uint64_t i = ((ObjUi64 const *)obj)->i;
        return i <= UINT32_MAX ? (uint32_t)i : 0u;
    }
    case OBJ_I64: {
        int64_t i = ((ObjI64 const *)obj)->i;
        return i <= UINT32_MAX && i >= 0 ? (uint32_t)i : 0u;
    }
    case OBJ_INT: {
        Int const *i = &((ObjInt const *)obj)->i;
        return int_is_range(i, 0, UINT32_MAX) == INT_WITHIN ? int_to_u32(i) : 0u;
        }
    default: return 0u;
    }
}

bool valuesEqual(ObjPtr a, ObjPtr b) {
    if (a == b) return true;

    Obj const *objA = osDeref(a);
    Obj const *objB = osDeref(b);

    if (objA == objB) return true;
    if (objA->objType != objB->objType) return false;
    switch (objA->objType) {
    case OBJ_BOOL:     return ((ObjBool const *)objA)->b == ((ObjBool const *)objB)->b;
    case OBJ_NIL:      return true;
    case OBJ_DOUBLE:   return ((ObjDouble const *)objA)->d == ((ObjDouble const *)objB)->d;
    case OBJ_I8:       return ((ObjI8 const *)objA)->i == ((ObjI8 const *)objB)->i;
    case OBJ_UI8:      return ((ObjUi8 const *)objA)->i == ((ObjUi8 const *)objB)->i;
    case OBJ_I16:      return ((ObjI16 const *)objA)->i == ((ObjI16 const *)objB)->i;
    case OBJ_UI16:     return ((ObjUi16 const *)objA)->i == ((ObjUi16 const *)objB)->i;
    case OBJ_I32:      return ((ObjI32 const *)objA)->i == ((ObjI32 const *)objB)->i;
    case OBJ_UI32:     return ((ObjUi32 const *)objA)->i == ((ObjUi32 const *)objB)->i;
    case OBJ_I64:      return ((ObjI64 const *)objA)->i == ((ObjI64 const *)objB)->i;
    case OBJ_UI64:     return ((ObjUi64 const *)objA)->i == ((ObjUi64 const *)objB)->i;
    case OBJ_ADDRESS:  return AS_ADDRESS(a) == AS_ADDRESS(b);
    default:           return false; // Unreachable.
    }
}

void printValue(ObjPtr p) {
    fprintValue(stdout, p);
}

void fprintValue(FILE* op, ObjPtr p) {
    Obj const *obj = osDeref(p);

    switch (obj->objType) {
    case OBJ_BOOL: FPRINTMSG(op, ((ObjBool const *)obj)->b ? "true" : "false"); break;
    case OBJ_NIL: FPRINTMSG(op, "nil"); break;
    case OBJ_DOUBLE: FPRINTMSG(op, "%#g", ((ObjDouble const *)obj)->d); break;
    case OBJ_I8: FPRINTMSG(op, "%d", ((ObjI8 const *)obj)->i); break;
    case OBJ_UI8: FPRINTMSG(op, "%u", ((ObjUi8 const *)obj)->i); break;
    case OBJ_I16: FPRINTMSG(op, "%d", ((ObjI16 const *)obj)->i); break;
    case OBJ_UI16: FPRINTMSG(op, "%u", ((ObjUi16 const *)obj)->i); break;
    case OBJ_I32: FPRINTMSG(op, "%d", ((ObjI32 const *)obj)->i); break;
    case OBJ_UI32: FPRINTMSG(op, "%u", ((ObjUi32 const *)obj)->i); break;
    case OBJ_I64: FPRINTMSG(op, "%" PRId64, ((ObjI64 const *)obj)->i); break;
    case OBJ_UI64: FPRINTMSG(op, "%" PRIu64, ((ObjUi64 const *)obj)->i); break;
    case OBJ_ADDRESS: FPRINTMSG(op, "%p", (void *)((ObjAddress const *)obj)->a); break;
    default: fprintObject(op, p); break;
    }
}

uintptr_t pinUniformArray(ObjPtr array) {
    ObjArray const *a = (ObjArray const *)osDeref(array);
    assert(a->obj.objType == OBJ_ARRAY);
    ObjPtr const *first = daAt(&a->elements, 0);
    Obj const *obj0 = osDeref(*first);

    size_t elementSize = 0u;
    uint8_t *elementOffset = 0;
    switch (obj0->objType) {
    case OBJ_BOOL: elementSize = sizeof (bool); elementOffset = (uint8_t *)&((ObjBool *)0)->b; break;
    case OBJ_DOUBLE: elementSize = sizeof (double); elementOffset = (uint8_t *)&((ObjDouble *)0)->d; break;
    case OBJ_I8: case OBJ_UI8: elementSize = sizeof (uint8_t); elementOffset = (uint8_t *)&((ObjI8 *)0)->i; break;
    case OBJ_I16: case OBJ_UI16: elementSize = sizeof (uint16_t); elementOffset = (uint8_t *)&((ObjI16 *)0)->i; break;
    case OBJ_I32: case OBJ_UI32: elementSize = sizeof (uint32_t); elementOffset = (uint8_t *)&((ObjI32 *)0)->i; break;
    case OBJ_I64: case OBJ_UI64: elementSize = sizeof (uint64_t); elementOffset = (uint8_t *)&((ObjI64 *)0)->i; break;
    case OBJ_ADDRESS: elementSize = sizeof (uintptr_t); elementOffset = (uint8_t *)&((ObjAddress *)0)->a; break;
    default: return 0;
    }

    for (int i = 1; i < a->elements.arrayLength; i++) {
        ObjPtr const *item = daAt(&a->elements, i);
        Obj const *obj = osDeref(*item);
        if (obj0->objType != obj->objType) {
            return 0;
        }
    }

    osNoGc(array);
    ObjPtr placedArray = ALLOCATE_OBJ(ObjPlacedArray, OBJ_PLACED_ARRAY);
    ObjPlacedArray *pa = (ObjPlacedArray *)osDeref(*first);
    osNoGc(placedArray);
    pa->blob = ALLOCATE_BLOB(elementSize, a->elements.arrayLength);
    ObjBlob *b = (ObjBlob *)osDeref(pa->blob);
    b->length = a->elements.arrayLength;

    for (int i = 0; i < a->elements.arrayLength; i++) {
        uintptr_t item = (uintptr_t)daAt(&a->elements, i);
        memcpy(b->memory + i * elementSize, item + elementOffset, elementSize);
    }

    osGcOk(placedArray);
    osGcOk(array);
    osNoGc(pa->blob);
    pa->placedAddress = (uintptr_t)b->memory;
    return pa->placedAddress;
}
