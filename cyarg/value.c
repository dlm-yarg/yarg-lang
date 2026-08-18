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
    ObjType t = osType(p);

    if (t == OBJ_UI32 || t == OBJ_UI16 || t == OBJ_UI8) {
        return true;
    } else if (t == OBJ_UI64) {
        ObjUi64 const *oi = osDeref(p);
        return oi->i <= UINT32_MAX;
    } else if (t == OBJ_I32) {
        ObjI32 const *oi = osDeref(p);
        return oi->i >= 0;
    } else if (t == OBJ_I16) {
        ObjI16 const *oi = osDeref(p);
        return oi->i >= 0;
    } else if (t == OBJ_I8) {
        ObjI8 const *oi = osDeref(p);
        return oi->i >= 0;
    } else if (t == OBJ_I64) {
        ObjI64 const *oi = osDeref(p);
        return oi->i >= 0 && oi->i <= UINT32_MAX;
    } else if (t == OBJ_INT) {
        ObjInt const *oi = osDeref(p);
        return int_is_range(&oi->i, 0, UINT32_MAX) == INT_WITHIN;
    }
    return false;
}

uint32_t as_positive_integer32(ObjPtr p) {
    ObjType t = osType(p);
    void const *obj = osDeref(p);

    switch (t) {
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
    if (a >= OBJ_PTR_TAGS || b >= OBJ_PTR_TAGS) return false;
    ObjType t = osType(a);
    if (t != osType(b)) return false;

    void const *objA = osDeref(a);
    void const *objB = osDeref(b);

    if (objA == objB) return true; // two ptrs to the same obj? assert

    switch (t) {
    case OBJ_BOOL:     return ((ObjBool const *)objA)->b == ((ObjBool const *)objB)->b;
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
    default:           assert(!"Do not call with type");
    }
}
