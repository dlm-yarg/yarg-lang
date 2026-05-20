#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "object.h"
#include "memory.h"
#include "value.h"
#include "yargtype.h"

ObjPtr copyValue(ObjPtr p) {
    
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
    ObjType t = obj->objType;

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

bool is_uniformarray(ObjPtr p) {
    Obj const *obj = osDeref(p);

    if (val.storedType == NULL) {
        return IS_UNIFORMARRAY(val.storedValue->asValue);
    } else if (val.storedType->yt == TypeArray) {
        return true;
    } else {
        return false;
    }
}

bool is_struct(PackedValue val) {
    if (val.storedType == NULL) {
        return IS_STRUCT(val.storedValue->asValue);
    } else if (val.storedType->yt == TypeStruct) {
        return true;
    } else {
        return false;
    }
}

bool is_nil(PackedValue val) {
    if (val.storedType == NULL) {
        return IS_NIL(val.storedValue->asValue);
    } else if (val.storedType->yt == TypeAny) {
        return IS_NIL(val.storedValue->asValue);
    } else if (type_packs_as_obj(val.storedType)
               && val.storedValue == NULL) {
        return true;
    } else {
        return false;
    }
}

bool is_channel(PackedValue val) {
    if (val.storedType == NULL) {
        return IS_CHANNEL(val.storedValue->asValue);
    } else if (val.storedType->yt == TypeChannel) {
        return true;
    } else {
        return false;
    }
}

uintptr_t pinUniformArray(ObjPtr array) {
    pinObj((Obj*)array);
    return (uintptr_t) array->store.storedValue;
}
