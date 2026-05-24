#ifndef cyarg_object_h
#define cyarg_object_h

#include "object_type.h"
#include "object_store.h"
#include "dynamic_array.h"
#include "platform_hal.h"
#include "big-int/big-int.h"
#include "chunk.h"

#include <stdio.h>

// Objects are stored in the object store

#define ALLOCATE_OBJ(type, objType) allocateObject(sizeof (type), (objType))
#define ALLOCATE_VAR_OBJ(type, objType, arrayMember, itemType, numItems) allocateVarObject(sizeof (type), (objType), sizeof (itemType), (size_t)(uint8_t *)&((type *)0)->arrayMember, (numItems))
#define EXTEND_VAR_OBJ(ptr, type, arrayMember, numItems) extendVarObject((ptr), sizeof (type), (size_t)(uint8_t *)&(((type) *)0)->(arrayMember), (numItems))
#define ALLOCATE_BLOB(itemSize, numItems) allocateObject(sizeof (ObjBlob) + (itemSize) * (numItems), OBJ_BLOB)

#define IS_ADDRESS(obj)      isObjOfType((obj), OBJ_ADDRESS)
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
#define IS_ARRAY(obj)        isObjOfOneType((obj), 2, OBJ_ARRAY, OBJ_PLACED_ARRAY)
#define IS_YARGTYPE(obj)     isObjOfOneType((obj), 5, OBJ_YARGTYPE, OBJ_YARGTYPE_ARRAY, OBJ_YARGTYPE_STRUCT, OBJ_YARGTYPE_POINTER, OBJ_YARGTYPE_MAP)
#define IS_POINTER(obj)      isObjOfType((obj), OBJ_POINTER) // OBJ_PACKED_VALUE?
#define IS_STRUCT(obj)       isObjOfOneType((obj), 2, OBJ_STRUCT, OBJ_PLACED_STRUCT)
#define IS_SYNCGROUP(obj)    isObjOfType((obj), OBJ_SYNCGROUP)
#define IS_MAP(obj)          isObjOfType((obj), OBJ_MAP)

#define AS_BOUND_METHOD(obj) ((ObjBoundMethod const *)osDeref(obj))
#define AS_CLASS(obj)        ((ObjClass const *)osDeref(obj))
#define AS_CLOSURE(obj)      ((ObjClosure const *)osDeref(obj))
#define AS_FUNCTION(obj)     ((ObjFunction const *)osDeref(obj))
#define AS_INSTANCE(obj)     ((ObjInstance const *)osDeref(obj))
#define AS_NATIVE(obj)       (((ObjNative const *)osDeref(obj))->function)
#define AS_ROUTINE(obj)      ((ObjRoutine const *)osDeref(obj))
#define AS_CHANNEL(obj)      ((ObjChannelContainer const *)osDeref(obj))
#define AS_STRING(obj)       ((ObjString const *)osDeref(obj))
#define AS_CSTRING(obj)      (((ObjString const *)osDeref(obj))->chars)
#define AS_ARRAY(obj)        ((ObjArray const *)osDeref(obj))
#define AS_YARGTYPE(obj)     ((ObjConcreteYargType const *)osDeref(obj))
#define AS_POINTER(obj)      ((ObjPointer const *)osDeref(obj))
#define AS_STRUCT(obj)       ((ObjStruct const *)osDeref(obj))
#define AS_SYNCGROUP(obj)    ((ObjSyncGroup const *)osDeref(obj))
#define AS_INTOBJ(obj)       ((ObjInt const *)osDeref(obj))
#define AS_INT(obj)          (&(AS_INTOBJ(obj)->i))
#define AS_MAP(obj)          ((ObjMap const *)osDeref(obj))
#define AS_NIL(obj)          ((ObjNil const *)osDeref(obj))
#define AS_BOOLOBJ(obj)      ((ObjBool const *)osDeref(obj)
#define AS_BOOL(obj)         ((ObjBool const *)osDeref(obj)->b)
#define AS_ADDRESSOBJ(obj)   ((ObjAddress const *)osDeref(obj))
#define AS_ADDRESS(obj)      (((ObjAddress const *)osDeref(obj))->a)
#define AS_DOUBLEOBJ(obj)    ((ObjDouble const *)osDeref(obj))
#define AS_DOUBLE(obj)       (((ObjDouble const *)osDeref(obj))->d)
#define AS_I8OBJ(obj)        ((ObjI8 const *)osDeref(obj))
#define AS_I8(obj)           (((ObjI8 const *)osDeref(obj))->i)
#define AS_UI8OBJ(obj)       ((ObjUi8 const *)osDeref(obj))
#define AS_UI8(obj)          (((ObjUi8 const *)osDeref(obj))->i)
#define AS_I16OBJ(obj)       ((ObjI16 const *)osDeref(obj))
#define AS_I16(obj)          (((ObjI16 const *)osDeref(obj))->i)
#define AS_UI16OBJ(obj)      ((ObjUi16 const *)osDeref(obj))
#define AS_UI16(obj)         (((ObjUi16 const *)osDeref(obj))->i)
#define AS_I32OBJ(obj)       ((ObjI32 const *)osDeref(obj))
#define AS_I32(obj)          (((ObjI32 const *)osDeref(obj))->i)
#define AS_UI32OBJ(obj)      ((ObjUi32 const *)osDeref(obj))
#define AS_UI32(obj)         (((ObjUi32 const *)osDeref(obj))->i)
#define AS_I64OBJ(obj)       ((ObjI64 const *)osDeref(obj))
#define AS_I64(obj)          (((ObjI64 const *)osDeref(obj))->i)
#define AS_UI64OBJ(obj)      ((ObjUi64 const *)osDeref(obj))
#define AS_UI64(obj)         (((ObjUi64 const *)osDeref(obj))->i)

// from yargtype.h
typedef struct ObjConcreteYargType ObjConcreteYargType;
typedef struct ObjConcreteYargTypeArray ObjConcreteYargTypeArray;
typedef struct ObjConcreteYargTypeStruct ObjConcreteYargTypeStruct;
typedef struct ObjConcreteYargTypePointer ObjConcreteYargTypePointer;
typedef struct ObjConcreteYargTypeMap ObjConcreteYargTypeMap;

typedef enum {
    OBJ_NIL,
    OBJ_BOOL,
    OBJ_INT,
    OBJ_ADDRESS, // ROM, DEVICE or BLOB;
    OBJ_DOUBLE,
    OBJ_I8,
    OBJ_UI8,
    OBJ_I16,
    OBJ_UI16,
    OBJ_I32,
    OBJ_UI32,
    OBJ_I64,
    OBJ_UI64,
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_ROUTINE,
    OBJ_CHANNELCONTAINER,
    OBJ_POINTER, // to any object including PLACED
    OBJ_BLOB, // either object store or PLACED (ROM, DEVICE or BLOB); BLOBS do not move during osExtend() or osGc() so allocate early
    OBJ_UPVALUE,
    OBJ_ARRAY,
    OBJ_MAP,
    OBJ_STRUCT,
    // placed objects reference system memory - ROM, DEVICE or BLOB; may only comprise: integral, float, system addresses, other placed objects
    OBJ_PLACED_ARRAY, // placed arrays have uniform elements
    OBJ_PLACED_STRUCT,
    OBJ_PLACED_VALUE, // can have a pointer to it, but it is not needed, this is equivalent to a pointer for placed values
    OBJ_YARGTYPE,
    OBJ_YARGTYPE_ARRAY,
    OBJ_YARGTYPE_STRUCT,
    OBJ_YARGTYPE_POINTER,
    OBJ_YARGTYPE_MAP,
    OBJ_SYNCGROUP,
    OBJ_STACKSLICE,

    // used by compiler and VM
    OBJ_STRING,

    // used by compiler only
    OBJ_PLACEALIAS,
    OBJ_AST,
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

    OBJ_IN_ROM = 0x80 // add to above to mark as XIP/ROM-Obj i.e. not in the object store
} ObjType;

_Static_assert(OBJ_EXPR_TYPE_INDEXED_COLLECTION < OBJ_IN_ROM, "run out of object names -- make objTyp uint16_t");

typedef struct Obj {
    uint8_t objType; // ObjType
} Obj;

typedef struct ObjString7 {
    Obj obj;
    uint32_t hash;
    struct {
        ArrayItemCount arrayCapacity;
        ArrayItemCount arrayLength;
        ObjSize arrayItemSize;
        union {
            uint64_t arrayItems[1];
            char chars[8];
        };
    } s;
} ObjString7;

typedef struct ObjString {
    Obj obj;
    ObjPtr nextSameHash;
    uint8_t hash;
    DynamicArray s; // of type char
} ObjString;

typedef struct ObjFunction {
    Obj obj;
    uint16_t arity;
    uint16_t upvalueCount;
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
    Int i;     // variable size
} ObjInt;

typedef struct ObjInt2 {
    Obj obj;
    bool isLiteral;
    IntConcrete2 i;
} ObjInt2;

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
    ObjPtr contents;
    size_t stackOffset;
    ObjPtr closed; // todo - what is this?
} ObjUpvalue;

typedef struct ObjClosure {
    Obj obj;
    ObjPtr function;
    DynamicArray upvalues; // of type ObjPtr
} ObjClosure;

typedef struct ObjClass {
    Obj obj;
    ObjPtr name;
    DynamicArray methods; // of type KeyValue
} ObjClass;

typedef struct ObjInstance {
    Obj obj;
    ObjPtr klass;
    DynamicArray fields; // of type KeyValue
} ObjInstance;

typedef struct ObjBoundMethod {
    Obj obj;
    ObjPtr receiver;
    ObjPtr method;
} ObjBoundMethod;

typedef struct ObjArray {
    Obj obj;
    DynamicArray elements; // of type ObjPtr
} ObjArray;

typedef struct ObjPlacedArray {
    Obj obj;
    ObjPtr type;
    ObjPtr blob; // if an ObjArray is copyed because it is pinned
    uintptr_t placedAddress;
    ObjSize elementSize;
} ObjPlacedArray;

typedef struct ObjPlacedStruct {
    Obj obj;
    uintptr_t placedAddress;
    DynamicArray elements; // of type KeyOffsetType
} ObjPlacedStruct;

typedef struct ObjPlacedValue {
    Obj obj;
    ObjPtr type;
    uintptr_t placedAddress;
} ObjPlacedValue;

typedef struct ObjBlob {
    Obj obj;
    ObjSize length;
    uint64_t memory[];
} ObjBlob;

typedef struct ObjPackedUniformArrayUnowned {
    Obj obj;
    ObjPtr type;
    ObjPtr location;
} ObjPackedUniformArrayUnowned;

typedef struct ObjPointer {
    Obj obj;
    ObjPtr type;
    ObjPtr destination;
} ObjPointer;

typedef struct ObjStruct {
    Obj obj;
    DynamicArray store; // of type KeyValue
} ObjStruct;

typedef struct ObjMap {
    Obj obj;
    DynamicArray entries; // of type KeyValue
} ObjMap;

typedef struct SyncElement {
    ObjPtr channel;
    ObjPtr result;
} SyncElement;

typedef struct ObjSyncGroup {
    Obj obj;
    platform_critical_section group_lock;
    DynamicArray channels; // of type SyncElement
} ObjSyncGroup;

ObjPtr allocateObject(size_t size, ObjType type);
ObjPtr allocateVarObject(size_t size, ObjType type, size_t itemSize, size_t arrayOffset, size_t numItems);
void extendVarObject(ObjPtr p, size_t size, size_t arrayOffset, size_t numItems);
ObjPtr allocateIntObject(size_t numDigits);

ObjPtr newBoundMethod(ObjPtr receiver, ObjPtr method);
ObjPtr newClass(ObjPtr name);
ObjPtr newClosure(ObjPtr function);
ObjPtr newFunction(void);
void initFunction(ObjPtr function);
ObjPtr newInstance(ObjPtr klass);
ObjPtr newNative(NativeFn function);
ObjPtr newArray(ObjPtr type);
ObjPtr newMap(ObjPtr type);
ObjPtr takeString(char *chars, int length);
ObjPtr copyString(const char *chars, int length);
ObjPtr newStringWithEscapes(const char *chars, int length);
ObjPtr newUpvalue(size_t stackOffset);
ObjPtr newInt(int64_t value);
ObjPtr newIntU(uint64_t value);

void *arrayAt(DynamicArray *array, size_t index);

ObjPtr structField(DynamicArray *struct_, size_t index);
bool structFieldIndex(ObjPtr, ObjPtr name, ArrayItemCount *index);

ObjPtr newPointerForHeapCell(ObjPtr type, ObjPtr location);
ObjPtr newPointerAtHeapCell(ObjPtr type, ObjPtr location);

ObjPtr destinationObject(ObjPtr pointer);
void offsetPointerDestination(ObjPtr, size_t offset);

ObjPtr defaultArray(ObjPtr type);
ObjPtr defaultStructValue(ObjPtr type);

ObjPtr placeObjectAt(ObjPtr type, ObjPtr location);

void printObject(ObjPtr);
void fprintObject(FILE *out, ObjPtr);

inline bool isObjOfType(ObjPtr obj, ObjType type) {
    Obj const *o = osDeref(obj);
    return o != 0 && o->objType == type;
}

bool isObjOfOneType(ObjPtr, size_t, ...);

bool isAddressValue(ObjPtr);

bool isArrayPointer(ObjPtr);
bool isStructPointer(ObjPtr);

#endif
