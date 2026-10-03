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
    ObjPtr type;
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

// ObjPtr newYargTypeFromType(ObjPtr yt); -- either just assign tag or use the appropriate of the following four functions
ObjPtr newYargArrayType(ObjPtr elementType);
ObjPtr newYargStructType(size_t fieldCount);
ObjPtr newYargPointerType(ObjPtr referenceType);
ObjPtr newYargMapType(ObjPtr keyType, ObjPtr valueType); // keyType is always string

void addFieldType(ObjConcreteYargTypeStruct* st, ObjPtr type, ObjPtr name);

bool isUint32Pointer(ObjPtr);

ObjPtr concrete_typeof(ObjPtr);
bool is_placeable_type(ObjPtr);
bool is_stored_type(ObjPtr);

ObjPtr defaultValue(ObjPtr type);

bool isSupportedMapKeyType(ObjPtr type);

void printType(FILE* op, ObjPtr type);

#endif
