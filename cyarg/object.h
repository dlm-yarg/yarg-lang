#ifndef cyarg_object_h
#define cyarg_object_h

#include "object_type.h"
#include "object_store.h"
#include "dynamic_array.h"
#include "big-int/big-int.h"
#include "chunk.h"

// Objects are stored on the object heap

// from yargtype.h
typedef struct ObjConcreteYargType ObjConcreteYargType;
typedef struct ObjConcreteYargTypeArray ObjConcreteYargTypeArray;
typedef struct ObjConcreteYargTypeStruct ObjConcreteYargTypeStruct;
typedef struct ObjConcreteYargTypePointer ObjConcreteYargTypePointer;
typedef struct ObjConcreteYargTypeMap ObjConcreteYargTypeMap;

#define IS_BOUND_METHOD(obj) isObjOfType((obj), OBJ_BOUND_METHOD)
#define IS_CLASS(obj)        isObjOfType((obj), OBJ_CLASS)
#define IS_CLOSURE(obj)      isObjOfType((obj), OBJ_CLOSURE)
#define IS_FUNCTION(obj)     isObjOfType((obj), OBJ_FUNCTION)
#define IS_INSTANCE(obj)     isObjOfType((obj), OBJ_INSTANCE)
#define IS_NATIVE(obj)       isObjOfType((obj), OBJ_NATIVE)
#define IS_BLOB(obj)         isObjOfType((obj), OBJ_BLOB)
#define IS_ROUTINE(obj)      isObjOfType((obj), OBJ_ROUTINE)
#define IS_CHANNEL(obj)      isObjOfType((obj), OBJ_CHANNELCONTAINER)
#define IS_STRING(obj)       isObjOfType((obj), OBJ_STRING)
#define IS_UNIFORMARRAY(obj) (isObjOfType((obj), OBJ_PACKEDUNIFORMARRAY) || isObjOfType((obj), OBJ_UNOWNED_UNIFORMARRAY))
#define IS_YARGTYPE(obj)     (isObjOfType((obj), OBJ_YARGTYPE) || isObjOfType((obj), OBJ_YARGTYPE_ARRAY) || isObjOfType((obj), OBJ_YARGTYPE_STRUCT) || isObjOfType((obj), OBJ_YARGTYPE_POINTER) || isObjOfType((obj), OBJ_YARGTYPE_MAP))
#define IS_POINTER(obj)      (isObjOfType((obj), OBJ_PACKEDPOINTER) || isObjOfType((obj), OBJ_UNOWNED_PACKEDPOINTER))
#define IS_STRUCT(obj)       (isObjOfType((obj), OBJ_PACKEDSTRUCT) || isObjOfType((obj), OBJ_UNOWNED_PACKEDSTRUCT))
#define IS_SYNCGROUP(obj)    isObjOfType((obj), OBJ_SYNCGROUP)
#define IS_MAP(obj)          isObjOfType((obj), OBJ_MAP)

#define AS_BOUND_METHOD(obj) ((ObjBoundMethod*)osDeref(obj))
#define AS_CLASS(obj)        ((ObjClass *)osDeref(obj))
#define AS_CLOSURE(obj)      ((ObjClosure *)osDeref(obj))
#define AS_FUNCTION(obj)     ((ObjFunction *)osDeref(obj))
#define AS_INSTANCE(obj)     ((ObjInstance *)osDeref(obj))
#define AS_NATIVE(obj)       (((ObjNative *)osDeref(obj))->function)
#define AS_ROUTINE(obj)      ((ObjRoutine *)osDeref(obj))
#define AS_CHANNEL(obj)      ((ObjChannelContainer *)osDeref(obj))
#define AS_STRING(obj)       ((ObjString *)osDeref(obj))
#define AS_CSTRING(obj)      (((ObjString *)osDeref(obj))->chars)
#define AS_UNIFORMARRAY(obj) ((ObjPackedUniformArray *)osDeref(obj))
#define AS_YARGTYPE(obj)     ((ObjConcreteYargType *)osDeref(obj))
#define AS_POINTER(obj)      ((ObjPackedPointer *)osDeref(obj))
#define AS_STRUCT(obj)       ((ObjPackedStruct *)osDeref(obj))
#define AS_SYNCGROUP(obj)    ((ObjSyncGroup *)osDeref(obj))
#define AS_INTOBJ(obj)       ((ObjInt *)osDeref(obj))
#define AS_INT(obj)          (&(AS_INTOBJ(obj)->bigInt))
#define AS_MAP(obj)          ((ObjMap *)osDeref(obj))
#define AS_NIL(obj)          ((ObjNil *)osDeref(obj))
#define AS_BOOLOBJ(obj)         ((ObjBool *)osDeref(obj)
#define AS_BOOL(obj)         ((ObjBool *)osDeref(obj)->b)
#define AS_ADDRESSOBJ(obj)      ((ObjAddress *)osDeref(obj))
#define AS_ADDRESS(obj)      (((ObjAddress *)osDeref(obj))->a)
#define AS_DOUBLEOBJ(obj)       ((ObjDouble *)osDeref(obj))
#define AS_DOUBLE(obj)       (((ObjDouble *)osDeref(obj))->d)
#define AS_I8OBJ(obj)           ((ObjI8 *)osDeref(obj))
#define AS_I8(obj)           (((ObjI8 *)osDeref(obj))->i)
#define AS_UI8OBJ(obj)          ((ObjUi8 *)osDeref(obj))
#define AS_UI8(obj)          (((ObjUi8 *)osDeref(obj))->i)
#define AS_I16OBJ(obj)          ((ObjI16 *)osDeref(obj))
#define AS_I16(obj)          (((ObjI16 *)osDeref(obj))->i)
#define AS_UI16OBJ(obj)         ((ObjUi16 *)osDeref(obj))
#define AS_UI16(obj)         (((ObjUi16 *)osDeref(obj))->i)
#define AS_I32OBJ(obj)          ((ObjI32 *)osDeref(obj))
#define AS_I32(obj)          (((ObjI32 *)osDeref(obj))->i)
#define AS_UI32OBJ(obj)         ((ObjUi32 *)osDeref(obj))
#define AS_UI32(obj)         (((ObjUi32 *)osDeref(obj))->i)
#define AS_I64OBJ(obj)          ((ObjI64 *)osDeref(obj))
#define AS_I64(obj)          (((ObjI64 *)osDeref(obj))->i)
#define AS_UI64OBJ(obj)         ((ObjUi64 *)osDeref(obj))
#define AS_UI64(obj)         (((ObjUi64 *)osDeref(obj))->i)

typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_ROUTINE,
    OBJ_CHANNELCONTAINER,
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_UNOWNED_UNIFORMARRAY,
    OBJ_PACKEDUNIFORMARRAY,
    OBJ_YARGTYPE,
    OBJ_YARGTYPE_ARRAY,
    OBJ_YARGTYPE_STRUCT,
    OBJ_YARGTYPE_POINTER,
    OBJ_YARGTYPE_MAP,
    OBJ_PACKEDPOINTER,
    OBJ_UNOWNED_PACKEDPOINTER,
    OBJ_UNOWNED_PACKEDSTRUCT,
    OBJ_PACKEDSTRUCT,
    OBJ_SYNCGROUP,
    OBJ_MAP,
    OBJ_STACKSLICE,
    OBJ_AST,
    OBJ_PLACEALIAS,
    OBJ_STMT_EXPRESSION,
    OBJ_STMT_PRINT,
    OBJ_STMT_POKE,
    OBJ_STMT_VARDECLARATION,
    OBJ_STMT_FIELDDECLARATION,
    OBJ_STMT_PLACEDECLARATION,
    OBJ_STMT_BLOCK,
    OBJ_STMT_IF,
    OBJ_STMT_FUNDECLARATION,
    OBJ_STMT_WHILE,
    OBJ_STMT_RETURN,
    OBJ_STMT_YIELD,
    OBJ_STMT_FOR,
    OBJ_STMT_CLASSDECLARATION,
    OBJ_EXPR_NUMBER,
    OBJ_EXPR_ADDRESS,
    OBJ_EXPR_OPERATION,
    OBJ_EXPR_GROUPING,
    OBJ_EXPR_NAMEDVARIABLE,
    OBJ_EXPR_LITERAL,
    OBJ_EXPR_STRING,
    OBJ_EXPR_CALL,
    OBJ_EXPR_COLLECTION_INITIALIZER,
    OBJ_EXPR_COLLECTION_ELEMENT,
    OBJ_EXPR_PAIR,
    OBJ_EXPR_BUILTIN,
    OBJ_EXPR_DOT,
    OBJ_EXPR_SUPER,
    OBJ_EXPR_TYPE,
    OBJ_EXPR_TYPE_STRUCT,
    OBJ_EXPR_TYPE_INDEXED_COLLECTION,
    OBJ_NIL,
    OBJ_BOOL,
    OBJ_INT,
    OBJ_ADDRESS,
    OBJ_DOUBLE,
    OBJ_I8,
    OBJ_UI8,
    OBJ_I16,
    OBJ_UI16,
    OBJ_I32,
    OBJ_UI32,
    OBJ_I64,
    OBJ_UI64
} ObjType;

typedef struct Obj {
    uint8_t objType; // ObjType
} Obj;

typedef struct ObjString {
    Obj obj;
    uint32_t hash;
    DynamicArray s; // of char
} ObjString;

typedef struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjPtr fName;
} ObjFunction;

typedef bool (*NativeFn)(ObjPtr routine, int argCount, ObjPtr *result);

typedef struct ObjNative {
    Obj obj;
    NativeFn function;
} ObjNative;

typedef struct ObjInt {
    Obj obj;
    bool isLiteral;
    Int bigInt;     // variable size
} ObjInt;

typedef struct ObjNil {
    Obj obj;
} ObjNil;
typedef struct ObjBool {
    Obj obj;
    bool b;
} ObjBool;
typedef struct ObjAddress {
    Obj obj;
    uintptr_t a;
} ObjAddress;
typedef struct ObjDouble {
    Obj obj;
    double d;
} ObjDouble;
typedef struct ObjI8 {
    Obj obj;
    int8_t i;
} ObjI8;
typedef struct ObjUi8 {
    Obj obj;
    uint8_t i;
} ObjUi8;
typedef struct ObjI16 {
    Obj obj;
    int16_t i;
} ObjI16;
typedef struct ObjUi16 {
    Obj obj;
    uint16_t i;
} ObjUi16;
typedef struct ObjI32 {
    Obj obj;
    int32_t i;
} ObjI32;
typedef struct ObjUi32 {
    Obj obj;
    uint32_t i;
} ObjUi32;
typedef struct ObjI64 {
    Obj obj;
    int64_t i;
} ObjI64;
typedef struct ObjUi64 {
    Obj obj;
    uint64_t i;
} ObjUi64;

typedef struct ObjUpvalue {
    Obj obj;
    ValueCell contents;
    size_t stackOffset;
    ValueCell closed;
    ObjPtr uvNext; // Captured upValues
} ObjUpvalue;

typedef struct ObjClosure {
    Obj obj;
    ObjPtr function;
    DynamicArray upvalues; // items ObjPtr
} ObjClosure;

typedef struct ObjClass {
    Obj obj;
    ObjPtr name;
    DynamicArray methods; // items ValueCollectionEntry
} ObjClass;

typedef struct ObjInstance {
    Obj obj;
    ObjPtr klass;
    DynamicArray fields; // items ValueCollectionEntry
} ObjInstance;

typedef struct ObjBoundMethod {
    Obj obj;
    ObjPtr reciever;
    ObjPtr method;
} ObjBoundMethod;

typedef struct ObjPackedUniformArray {
    Obj obj;
    ObjPtr type;
    DynamicArray fields; // items ObjPtr
} ObjPackedUniformArray;

typedef struct ObjPackedPointer {
    Obj obj;
    ObjPtr type;
    ObjPtr destination;
} ObjPackedPointer;

typedef struct ObjPackedStruct {
    Obj obj;
    DynamicArray store; // items TypedCollectionEntry // todo + offset?
} ObjPackedStruct;

typedef struct ObjMap {
    Obj obj;
    DynamicArray entries; // items TypedCollectionEntry
} ObjMap;

#define ALLOCATE_OBJ(type, objectType) allocateObject(sizeof(type), (objectType))
#define ALLOCATE_VAR_OBJ(type, objectType, itemType, numItems) allocateObject(sizeof (type) + sizeof (itemType) * (numItems), (objectType))

ObjPtr allocateObject(size_t size, ObjType type);
ObjPtr allocateVarObject(size_t size, ObjType type, size_t itemSize, size_t numItems);
ObjPtr allocateIntObject(size_t numDigits);

ObjPtr newBoundMethod(ObjPtr receiver, ObjPtr method);
ObjPtr newClass(ObjPtr name);
ObjPtr newClosure(ObjPtr function);
ObjPtr newFunction(void);
void initFunction(ObjPtr function);
ObjPtr newInstance(ObjPtr klass);
ObjPtr newNative(NativeFn function);
ObjPtr newPackedUniformArray(ObjPtr type);
ObjPtr newMap(ObjPtr type);
ObjPtr takeString(char *chars, int length);
ObjPtr copyString(const char *chars, int length);
ObjPtr copyStringWithEscapes(const char *chars, int length);
ObjPtr newUpvalue(ValueCell *slot, size_t stackOffset);
ObjPtr newInt(int64_t value);
ObjPtr newIntU(uint64_t value);

ObjPtr arrayElement(DynamicArray *array, size_t index);
size_t arrayCardinality(DynamicArray *array);

ObjPtr structField(DynamicArray *struct_, size_t index);
bool structFieldIndex(ObjConcreteYargType *type, ObjString *name, size_t *index);
ObjPtr newPackedStructAt(DynamicArray *location);

ObjPtr newPointerForHeapCell(ObjPtr location);
ObjPtr newPointerAtHeapCell(ObjPtr location);

ObjPtr destinationObject(ObjPtr pointer);
void offsetPointerDestination(ObjPackedPointer *pointer, size_t offset);

ObjPtr newPackedUniformArrayAt(ObjPtr location);

void defaultIntValue(ObjPtr r);

ObjPtr defaultArray(ObjPtr type);
ObjPtr defaultStructValue(ObjPtr type);

ObjPtr placeObjectAt(ObjPtr type, ObjPtr location);

void printObject(ObjPtr);
void fprintObject(FILE *op, ObjPtr);

static inline bool isObjOfType(ObjPtr obj, ObjType type) {
    Obj *o = osDeref(obj);
    return o != 0 && o->objType == type;
}

bool isAddressValue(ObjPtr);

bool isArrayPointer(ObjPtr);
bool isStructPointer(ObjPtr);

#endif
