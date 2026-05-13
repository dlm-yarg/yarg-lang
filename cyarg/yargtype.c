#include "common.h"

#include "yargtype.h"
#include "memory.h"
#include "vm.h"

#include <assert.h>

ObjPtr newYargTypeFromType(ConcreteYargType yt) {
    ObjPtr r;
    switch (yt) {
    case TypeAny:
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
    case TypeString:
    case TypeClass:
    case TypeInstance:
    case TypeFunction:
    case TypeRoutine:
    case TypeChannel:
    case TypeYargType:
    case TypeInt :
        r = ALLOCATE_OBJ(ObjConcreteYargType, OBJ_YARGTYPE);
        AS_YARGTYPE(r)->yt = yt;
        break;
    case TypeArray:
        r = ALLOCATE_OBJ(ObjConcreteYargTypeArray, OBJ_YARGTYPE_ARRAY);
        AS_YARGTYPE_ARRAY(r)->core.yt = yt;
        break;
    case TypeStruct:
        assert(!"alloc struct using newYargStructType()");
    case TypePointer:
        r = ALLOCATE_OBJ(ObjConcreteYargTypePointer, OBJ_YARGTYPE_POINTER);
        AS_YARGTYPE_POINTER(r)->core.yt = yt;
        break;
    case TypeMap:
        r= ALLOCATE_OBJ(ObjConcreteYargTypeMap, OBJ_YARGTYPE_MAP);
        AS_YARGTYPE_MAP(r)->core.yt = yt;
    }
    return r;
}

ObjPtr newYargArrayTypeFromType(ObjPtr elementType) {
    ObjPtr r = newYargTypeFromType(TypeArray);
    ObjType t = osDeref(elementType)->objType;
    if (t == OBJ_YARGTYPE || t == OBJ_YARGTYPE_ARRAY || t == OBJ_YARGTYPE_STRUCT || t == OBJ_YARGTYPE_POINTER || t == OBJ_YARGTYPE_MAP) {
        AS_YARGTYPE_ARRAY(r)->element_type = elementType;
    }
    return r;
}

ObjPtr arrayElementType(ObjConcreteYargTypeArray* arrayType) {
    return arrayType->element_type ? arrayType->element_type : 0;
}

size_t arrayElementOffset(ObjConcreteYargTypeArray* arrayType, size_t index) {
    return index * arrayElementSize(arrayType);
}

size_t arrayElementSize(ObjConcreteYargTypeArray* arrayType) {
    return yt_sizeof_type_storage(arrayElementType(arrayType));
}

ObjPtr newYargStructType(size_t fieldCount) {
    ObjPtr r = ALLOCATE_VAR_OBJ(ObjConcreteYargTypeStruct, OBJ_YARGTYPE_STRUCT, YargTypeStructElement, fieldCount);
    ObjConcreteYargTypeStruct* s = AS_YARGTYPE_STRUCT(r);
    s->core.yt = TypeStruct;
    daInit(&s->elements, sizeof (YargTypeStructElement));
    return r;
}

void addFieldType(ObjConcreteYargTypeStruct *st, size_t index, ObjPtr type, ObjPtr offset, ObjPtr name) {
    assert(st->elements.arrayLength > index && st->elements.arrayItemSize == sizeof (YargTypeStructElement));
    YargTypeStructElement *e = &((YargTypeStructElement *)st->elements.arrayItems)[index];
    e->type = type;
    e->offset = offset;
    e->name = name;
    if (st->elements.arrayLength == 1 + index) {
        assert(st->storage_size == 0u);
        for (int i = 0; i < st->elements.arrayLength; i++) {
            e = &((YargTypeStructElement *)st->elements.arrayItems)[i];
            size_t offset = e->offset != 0 ? st->storage_size : (size_t)int_to_i64(AS_INT(e->offset));
            size_t element_size = yt_sizeof_type_storage(e->type);
            st->storage_size = offset + element_size;
        }
    }
}

ObjPtr newYargPointerType(ObjPtr targetType) {
    ObjPtr r =  newYargTypeFromType(TypePointer);
    if (IS_YARGTYPE(targetType)) {
        AS_YARGTYPE_POINTER(r)->target_type = targetType;
    }
    return r;
}

bool isUint32Pointer(ObjPtr val) {
    ObjPackedPointer *pointer = AS_POINTER(val);
    ObjType t = pointer->obj.objType;
    if (pointer->type != 0 && (t == OBJ_PACKEDPOINTER || t == OBJ_UNOWNED_PACKEDPOINTER)) {
        ObjConcreteYargType* dest = AS_YARGTYPE(pointer->type);
        return dest->yt == TypeUint32;
    }
    return false;
}

ObjPtr concrete_typeof(ObjPtr a) {
    Obj *o = osDeref(a);
    ObjType t = o->objType;
    switch (t) {
    case OBJ_NIL: return 0;
    case OBJ_BOOL: return newYargTypeFromType(TypeBool);
    case OBJ_INT: return newYargTypeFromType(TypeInt);
    case OBJ_ADDRESS: fatalVMError("Unexpected object type: address"); return 0;
    case OBJ_DOUBLE: return newYargTypeFromType(TypeDouble);
    case OBJ_I8: return newYargTypeFromType(TypeInt8);
    case OBJ_UI8: return newYargTypeFromType(TypeUint8);
    case OBJ_I16: return newYargTypeFromType(TypeInt16);
    case OBJ_UI16: return newYargTypeFromType(TypeUint16);
    case OBJ_I32: return newYargTypeFromType(TypeInt32);
    case OBJ_UI32: return newYargTypeFromType(TypeUint32);
    case OBJ_I64: return newYargTypeFromType(TypeInt64);
    case OBJ_UI64: return newYargTypeFromType(TypeUint64);
    case OBJ_FUNCTION: return newYargTypeFromType(TypeFunction);
    case OBJ_CLOSURE: return newYargTypeFromType(TypeFunction);
    case OBJ_NATIVE: return newYargTypeFromType(TypeFunction);
    case OBJ_BOUND_METHOD: return newYargTypeFromType(TypeFunction);
    case OBJ_CLASS: return newYargTypeFromType(TypeClass);
    case OBJ_INSTANCE: return newYargTypeFromType(TypeInstance);
    case OBJ_ROUTINE: return newYargTypeFromType(TypeRoutine);
    case OBJ_CHANNELCONTAINER: return newYargTypeFromType(TypeChannel);
    case OBJ_STRING: return newYargTypeFromType(TypeString);
    case OBJ_PACKEDUNIFORMARRAY: case OBJ_UNOWNED_UNIFORMARRAY: return AS_UNIFORMARRAY(a)->type;
    case OBJ_PACKEDSTRUCT: case OBJ_UNOWNED_PACKEDSTRUCT: return newYargTypeFromType(TypeStruct); // todo - stored type?
    case OBJ_YARGTYPE: case OBJ_YARGTYPE_ARRAY: case OBJ_YARGTYPE_STRUCT: case OBJ_YARGTYPE_POINTER: case OBJ_YARGTYPE_MAP:
        return newYargTypeFromType(TypeYargType);
    case OBJ_PACKEDPOINTER: case OBJ_UNOWNED_PACKEDPOINTER: return AS_POINTER(a)->type;
    case OBJ_MAP: return newYargTypeFromType(TypeMap);
    default:
        fatalVMError("Unexpected object type: %d", t);
        return 0;
    }
}

bool type_packs_as_obj(ObjConcreteYargType* type) {
    switch (type->yt) {
        case TypeAny:
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
        case TypeArray:
        case TypeStruct:
            return false;
        case TypeInt:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypePointer:
        case TypeMap:
        case TypeYargType:
            return true;
    }
}

bool type_packs_as_container(ObjConcreteYargType* type) {
    switch (type->yt) {
        case TypeAny:
        case TypeBool:
        case TypeInt:
        case TypeDouble:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeMap:
        case TypeYargType:
            return false;
        case TypePointer:
        case TypeArray:
        case TypeStruct:
            return true;
    }
}

bool is_nil_assignable_type(ObjPtr type) {
    if (type == 0) {
        return true;
    } else {
        Obj *o = osDeref(type);
        ObjType t = o->objType;
        if (t == OBJ_YARGTYPE || t == OBJ_YARGTYPE_ARRAY || t == OBJ_YARGTYPE_STRUCT ||
            t == OBJ_YARGTYPE_POINTER || t == OBJ_YARGTYPE_MAP) {
            ObjConcreteYargType* ct = (ObjConcreteYargType *)o;
            switch (ct->yt) {
            case TypeBool:
            case TypeInt:
            case TypeDouble:
            case TypeInt8:
            case TypeUint8:
            case TypeInt16:
            case TypeUint16:
            case TypeInt32:
            case TypeUint32:
            case TypeInt64:
            case TypeUint64:
            case TypeStruct:
                return false;
            case TypeAny:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeRoutine:
            case TypeChannel:
            case TypeArray:
            case TypePointer:
            case TypeMap:
            case TypeYargType:
                return true;
            }
        } else {
            return false;
        }
    }
}

bool is_placeable_type(ObjPtr typeVal) {
    ObjConcreteYargType *o = AS_YARGTYPE(typeVal);
    ObjType t = o->obj.objType;
    if (t == OBJ_YARGTYPE || t == OBJ_YARGTYPE_ARRAY || t == OBJ_YARGTYPE_STRUCT ||
        t == OBJ_YARGTYPE_POINTER || t == OBJ_YARGTYPE_MAP) {
        switch(o->yt) {
            case TypeInt8: return true;
            case TypeUint8: return true;
            case TypeInt16: return true;
            case TypeUint16: return true;
            case TypeInt32: return true;
            case TypeUint32: return true;
            case TypeInt64: return true;
            case TypeUint64: return true;
            case TypeArray: {
                ObjConcreteYargTypeArray* ct = (ObjConcreteYargTypeArray*)o;
                ObjPtr elementType = arrayElementType(ct);
                return is_placeable_type(elementType);
            }
            case TypeStruct: {
                ObjConcreteYargTypeStruct* ct = (ObjConcreteYargTypeStruct*)o;
                bool is_placeable = true;
                for (size_t i = 0; i < ct->elements.arrayLength; i++) {
                    YargTypeStructElement *e = &((YargTypeStructElement *)ct->elements.arrayItems)[i];
                    ObjPtr fieldType = e == 0 ? 0 : e->type;
                    is_placeable &= is_placeable_type(fieldType);
                }
                return is_placeable;
            }
            default: return false;
        }
    }
    return false;
}

bool is_stored_type(ObjPtr type) {
    ObjConcreteYargType *o = AS_YARGTYPE(type);
    ObjType t = o->obj.objType;
    if (t == OBJ_YARGTYPE || t == OBJ_YARGTYPE_ARRAY || t == OBJ_YARGTYPE_STRUCT ||
        t == OBJ_YARGTYPE_POINTER || t == OBJ_YARGTYPE_MAP) {
        switch(o->yt) {
        case TypeArray:
        case TypeStruct:
        case TypePointer:
            return true;
        default:
            return false;
        }
    }
    return false;
}

// todo - this needs to be looked at - only really needed when placed objects are read/written
size_t yt_sizeof_type_storage(ObjPtr type) {
    if (type == 0) {
        return sizeof (ObjNil);
    } else {
        ObjConcreteYargType* t = AS_YARGTYPE(type);
        switch (t->yt) {
        case TypeAny:
            return sizeof (ObjNil);
        case TypeBool:
            return sizeof (ObjBool);
        case TypeDouble:
            return sizeof (ObjDouble);
        case TypeInt8:
            return sizeof (ObjI8);
        case TypeUint8:
            return sizeof (ObjUi8);
        case TypeInt16:
            return sizeof (ObjI16);
        case TypeUint16:
            return sizeof (ObjUi16);
        case TypeInt32:
            return sizeof (ObjI32);
        case TypeUint32:
            return sizeof (ObjUi32);
        case TypeInt64:
            return sizeof (ObjI64);
        case TypeUint64:
            return sizeof (ObjUi64);
        case TypeStruct: {
            ObjConcreteYargTypeStruct* st = (ObjConcreteYargTypeStruct*)t;
            return st->storage_size;
        }
        case TypeArray: {
            ObjConcreteYargTypeArray* array = (ObjConcreteYargTypeArray*)t;
            return arrayElementSize(array) * array->cardinality;
        }
        case TypeInt:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypePointer:
        case TypeMap:
        case TypeYargType:
            return sizeof(Obj*);
        }
    }
}

// todo - pre alloc defaults
ObjPtr defaultValue(ObjPtr type) {
    if (type == 0) {
        return 0;
    } else {
        ObjConcreteYargType* ct = AS_YARGTYPE(type);
        switch (ct->yt) {
        case TypeBool: return ALLOCATE_OBJ(ObjBool, OBJ_BOOL);
        case TypeInt: return newIntU(0);
        case TypeDouble: return ALLOCATE_OBJ(ObjDouble, OBJ_DOUBLE);
        case TypeInt8: return ALLOCATE_OBJ(ObjI8, OBJ_I8);
        case TypeUint8: return ALLOCATE_OBJ(ObjUi8, OBJ_UI8);
        case TypeInt16: return ALLOCATE_OBJ(ObjI16, OBJ_I16);
        case TypeUint16: return ALLOCATE_OBJ(ObjUi16, OBJ_UI16);
        case TypeInt32: return ALLOCATE_OBJ(ObjI32, OBJ_I32);
        case TypeUint32: return ALLOCATE_OBJ(ObjUi32, OBJ_UI32);
        case TypeInt64: return ALLOCATE_OBJ(ObjI64, OBJ_I64);
        case TypeUint64: return ALLOCATE_OBJ(ObjUi64, OBJ_UI64);
        case TypeStruct: return defaultStructValue(type);
        case TypeArray: return defaultArray(type);
        case TypePointer:
        case TypeAny:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeMap:
        case TypeYargType:
            return 0;
        }
    }
}

static bool isAssignableCardinality(size_t lhsCardinality, size_t rhsCardinality) {
    if (lhsCardinality == 0) {
        return true;
    } else {
        return lhsCardinality == rhsCardinality;
    }
}

static bool isInitializableArray(ObjConcreteYargTypeArray* lhsConcreteType, ObjConcreteYargTypeArray* rhsConcreteType) {

    if (isAssignableCardinality(lhsConcreteType->cardinality, rhsConcreteType->cardinality)) {
        if (lhsConcreteType->element_type == 0) {
            return true;
        } else if (AS_YARGTYPE(lhsConcreteType->element_type)->yt == TypeAny && rhsConcreteType->element_type == 0) {
            return true;
        } else if (rhsConcreteType->element_type == 0) {
            return false;
        } else {
            return AS_YARGTYPE(lhsConcreteType->element_type)->yt == AS_YARGTYPE(rhsConcreteType->element_type)->yt;
        }
    } else {
        return false;
    }
}

bool isInitialisableType(ObjPtr lhsType, ObjPtr rhsValue, ObjPtr *promotedRhs) {

    promotedRhs = 0;
    ObjConcreteYargType *lhsConcreteType = AS_YARGTYPE(lhsType);
    if (lhsConcreteType->yt == TypeAny) {
        return true;
    }
    
    if (rhsValue == 0) {
        return is_nil_assignable_type(lhsType);
    }

    ObjPtr rhsType = concrete_typeof(rhsValue);
    ObjConcreteYargType* rhsConcreteType = AS_YARGTYPE(rhsType);

    if (lhsConcreteType->yt == TypeArray && rhsConcreteType->yt == TypeArray) {
        return isInitializableArray((ObjConcreteYargTypeArray*)lhsConcreteType, (ObjConcreteYargTypeArray*)rhsConcreteType);
    } else {
        if (rhsConcreteType->yt == TypeInt)
        {

            ObjInt *i = AS_INTOBJ(rhsValue);
            if (i->isLiteral)
            {
                switch (lhsConcreteType->yt)
                {
                case TypeInt8:
                    if (int_is_range(&i->bigInt, INT8_MIN, INT8_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjI8, OBJ_I8);
                        AS_I8OBJ(*promotedRhs)->i = int_to_i32(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeUint8:
                    if (int_is_range(&i->bigInt, 0, UINT8_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjUi8, OBJ_UI8);
                        AS_UI8OBJ(*promotedRhs)->i = int_to_u32(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeInt16:
                    if (int_is_range(&i->bigInt, INT16_MIN, INT16_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjI16, OBJ_I16);
                        AS_I16OBJ(*promotedRhs)->i = int_to_i32(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeUint16:
                    if (int_is_range(&i->bigInt, 0, UINT16_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjUi16, OBJ_UI16);
                        AS_UI16OBJ(*promotedRhs)->i = int_to_u32(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeInt32:
                    if (int_is_range(&i->bigInt, INT32_MIN, INT32_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjI32, OBJ_I32);
                        AS_I32OBJ(*promotedRhs)->i = int_to_i32(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeUint32:
                    if (int_is_range(&i->bigInt, 0, UINT32_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjUi32, OBJ_UI32);
                        AS_UI32OBJ(*promotedRhs)->i = int_to_u32(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeInt64:
                    if (int_is_range(&i->bigInt, INT64_MIN, INT64_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjI64, OBJ_I64);
                        AS_I64OBJ(*promotedRhs)->i = int_to_i64(&i->bigInt);
                        return true;
                    }
                    break;
                case TypeUint64:
                    if (int_is_range(&i->bigInt, 0, UINT64_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = ALLOCATE_OBJ(ObjUi64, OBJ_UI64);
                        AS_UI64OBJ(*promotedRhs)->i = int_to_u64(&i->bigInt);
                        return true;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        return lhsConcreteType->yt == rhsConcreteType->yt;
    }
}

// this is a temporary measure, until we have a more complete hashing setup.
bool isSupportedMapKeyType(ObjPtr type) {
    if (IS_YARGTYPE(type)) {
        switch (AS_YARGTYPE(type)->yt) {
            case TypeMap: {
                ObjConcreteYargTypeMap* mt = (ObjConcreteYargTypeMap*)AS_YARGTYPE(type);
                return isSupportedMapKeyType(mt->key_type == 0 ? 0 : mt->key_type);
            }
            case TypeString:
                return true;
            default:
                return false;
        }
    } else {
         return false;
    }
}

static void printTypeLiteral(FILE* op, ObjPtr type) {
    if (type == 0) {
        FPRINTMSG(op, "any");
        return;
    }

    ObjConcreteYargType *t = AS_YARGTYPE(type);
    switch (t->yt) {
        case TypeAny: FPRINTMSG(op, "any"); break;
        case TypeBool: FPRINTMSG(op, "bool"); break;
        case TypeDouble: FPRINTMSG(op, "mfloat64"); break;
        case TypeInt: FPRINTMSG(op, "int"); break;
        case TypeInt8: FPRINTMSG(op, "int8"); break;
        case TypeUint8: FPRINTMSG(op, "uint8"); break;
        case TypeInt16: FPRINTMSG(op, "int16"); break;
        case TypeUint16: FPRINTMSG(op, "uint16"); break;
        case TypeInt32: FPRINTMSG(op, "int32"); break;
        case TypeUint32: FPRINTMSG(op, "uint32"); break;
        case TypeInt64: FPRINTMSG(op, "int64"); break;
        case TypeUint64: FPRINTMSG(op, "uint64"); break;
        case TypeString: FPRINTMSG(op, "string"); break;
        case TypeClass: FPRINTMSG(op, "Class"); break;
        case TypeInstance: FPRINTMSG(op, "Instance"); break;
        case TypeFunction: FPRINTMSG(op, "Function"); break;
        case TypeRoutine: FPRINTMSG(op, "Routine"); break;
        case TypeChannel: FPRINTMSG(op, "Channel"); break;  
        case TypeYargType: FPRINTMSG(op, "Type"); break;
        case TypeArray: {
            ObjConcreteYargTypeArray* array = (ObjConcreteYargTypeArray*) t;
            ObjPtr type = arrayElementType(array);
            if (type == 0) {
                FPRINTMSG(op, "any");
            } else {
                printTypeLiteral(op, array->element_type);
            }
            FPRINTMSG(op, "[");
            if (array->cardinality > 0) {
                FPRINTMSG(op, "%zu", array->cardinality);
            }
            FPRINTMSG(op, "]");
            break;
        }
        case TypeStruct: {
            ObjConcreteYargTypeStruct* st = (ObjConcreteYargTypeStruct *) t;
            FPRINTMSG(op, "struct{|%u:%zu| ", (unsigned) st->elements.arrayLength, st->storage_size);
            for (ArrayItemCount i = 0; i < st->elements.arrayLength; i++) {
                printTypeLiteral(op, ((YargTypeStructElement *)st->elements.arrayItems)[i].type);
                FPRINTMSG(op, "; ");
            }
            FPRINTMSG(op, "}");
            break;
        }
        case TypePointer: {
            ObjConcreteYargTypePointer* st = (ObjConcreteYargTypePointer *) t;
            FPRINTMSG(op, "*");
            if (st->target_type) {
                printTypeLiteral(op, st->target_type);
            } else {
                FPRINTMSG(op, "any");
            }
            break;
        }
        case TypeMap: {
            ObjConcreteYargTypeMap* mt = (ObjConcreteYargTypeMap *) t;
            printTypeLiteral(op, mt->value_type);
            FPRINTMSG(op, "[");
            printTypeLiteral(op, mt->key_type);
            FPRINTMSG(op, "]");
            break;
        }
        default: FPRINTMSG(op, "Unknown"); break;
    }
}

void printType(FILE* op, ObjPtr type) {
    FPRINTMSG(op, "Type:");
    printTypeLiteral(op, type);
}
