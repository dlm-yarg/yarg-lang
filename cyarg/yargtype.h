#ifndef cyarg_yargtype_h
#define cyarg_yargtype_h

#include "object.h"
#include "stdio.h"

#define AS_YARGTYPE_ARRAY(obj)      ((ObjConcreteYargTypeArray const *)osDeref(obj))
#define AS_YARGTYPE_STRUCT(obj)     ((ObjConcreteYargTypeStruct const *)osDeref(obj))
#define AS_YARGTYPE_POINTER(obj)    ((ObjConcreteYargTypePointer const *)osDeref(obj))
#define AS_YARGTYPE_MAP(obj)        ((ObjConcreteYargTypeMap const *)osDeref(obj))

typedef struct ObjConcreteYargTypeArray {
    size_t cardinality;
    ObjPtr element_type; // OBJ_PTR_NIL for any
} ObjConcreteYargTypeArray;

typedef struct YargTypeStructElement {
    ObjPtr name; // of type ObjString
    ObjPtr type; // of type ObjConcreteYargType, ObjConcreteYargTypeArray, ObjConcreteYargTypeStruct, ObjConcreteYargTypePointer, ObjConcreteYargTypeMap
} YargTypeStructElement;

typedef struct ObjConcreteYargTypeStruct {
    DynamicArray elements; // of type YargTypeStructElement
} ObjConcreteYargTypeStruct;

typedef struct ObjConcreteYargTypePointer {
    ObjPtr target_type;
} ObjConcreteYargTypePointer;

typedef struct ObjConcreteYargTypeMap {
    ObjPtr key_type; // always string
    ObjPtr value_type;
} ObjConcreteYargTypeMap;

ObjPtr newYargTypeFromType(ObjPtr yt);

ObjPtr newYargArrayTypeFromType(ObjPtr elementType);
ObjPtr newYargStructType(size_t fieldCount);
ObjPtr newYargPointerType(ObjPtr targetType);

size_t arrayElementOffset(ObjConcreteYargTypeArray* arrayType, size_t index);
size_t arrayElementSize(ObjConcreteYargTypeArray* arrayType);
ObjPtr arrayElementType(ObjConcreteYargTypeArray* arrayType);

void addFieldType(ObjConcreteYargTypeStruct* st, size_t index, ObjPtr type, ObjPtr offset, ObjPtr name);

bool isUint32Pointer(ObjPtr);

ObjPtr concrete_typeof(ObjPtr);
bool type_packs_as_obj(ObjConcreteYargType* type);
bool type_packs_as_container(ObjConcreteYargType* type);
bool is_nil_assignable_type(ObjPtr);
bool is_placeable_type(ObjPtr);
bool is_stored_type(ObjPtr);
size_t yt_sizeof_type_storage(ObjPtr);

ObjPtr defaultValue(ObjPtr type);

bool isInitialisableType(ObjPtr lhsType, ObjPtr rhsValue, ObjPtr *promotedRhs); // promotedRhs will be VAL_NIL if no promotion

bool isSupportedMapKeyType(ObjPtr type);

void printType(FILE* op, ObjPtr type);

#endif
