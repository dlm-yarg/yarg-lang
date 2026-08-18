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
#define IS_YARGTYPE(obj)     (obj >= OBJ_PTR_TYPE_TAGS_ && obj < OBJ_PTR_TYPE_TAGS_END || isObjOfOneType((obj), 4, OBJ_YARGTYPE_ARRAY, OBJ_YARGTYPE_STRUCT, OBJ_YARGTYPE_POINTER, OBJ_YARGTYPE_MAP))
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

// if sLength == sCapacity the string will need to be expanded (if OBJ_STRING)
//                                      or copied (if OBJ_IN_ROM+OBJ_STRING) before using as a C string
typedef struct ObjString {
    ObjPtr sameHash; // null terminated linked list; sameHash is volatile
    ArrayItemCount sLength;
    char chars[0]; // null terminated if sCapacity > sLength
} ObjString;

typedef struct ObjRomString {
    ObjPtr sameHash; // null terminated linked list; sameHash is volatile
    char const *cStr; // null terminated
} ObjRomString;

typedef struct ObjFunction {
    uint16_t arity;
    uint16_t upvalueCount;
    Chunk chunk;
    ObjPtr fName;
} ObjFunction;

typedef bool (*NativeFn)(ObjPtr routine, int argCount, ObjPtr *result);

typedef struct ObjNative {
    NativeFn function;
} ObjNative;

typedef struct ObjInt {
    bool isLiteral;
    Int i;     // variable size
} ObjInt;

typedef struct ObjInt2 {
    bool isLiteral;
    IntConcrete2 i;
} ObjInt2;

typedef struct ObjBool {
    bool b;
} ObjBool;

typedef struct ObjAddress {
    uintptr_t a;
} ObjAddress;

typedef struct ObjDouble {
    double d;
} ObjDouble;

typedef struct ObjI8 {
    int8_t i;
} ObjI8;

typedef struct ObjUi8 {
    uint8_t i;
} ObjUi8;

typedef struct ObjI16 {
    int16_t i;
} ObjI16;

typedef struct ObjUi16 {
    uint16_t i;
} ObjUi16;

typedef struct ObjI32 {
    int32_t i;
} ObjI32;

typedef struct ObjUi32 {
    uint32_t i;
} ObjUi32;

typedef struct ObjI64 {
    int64_t i;
} ObjI64;

typedef struct ObjUi64 {
    uint64_t i;
} ObjUi64;

typedef struct ObjUpvalue {
    ObjPtr contents;
    size_t stackOffset;
    ObjPtr closed; // todo - what is this?
} ObjUpvalue;

typedef struct ObjClosure {
    ObjPtr function;
    DynamicArray upvalues; // of type ObjPtr
} ObjClosure;

typedef struct ObjClass {
    ObjPtr name;
    DynamicArray methods; // of type KeyValue
} ObjClass;

typedef struct ObjInstance {
    ObjPtr klass;
    DynamicArray fields; // of type KeyValue
} ObjInstance;

typedef struct ObjBoundMethod {
    ObjPtr receiver;
    ObjPtr method; // of type ObjClosure
} ObjBoundMethod;

typedef struct ObjArray {
    DynamicArray elements; // of type ObjPtr (Any, struct, array etc) or integral type
} ObjArray;

typedef struct ObjPlaced {
    uintptr_t placedAddress;
    ObjPtr placedType; // YARG TYPE
} ObjPlaced;

typedef struct ObjBlob {
    uint64_t memory[];
} ObjBlob;

typedef struct ObjPointer {
    ObjPtr type; // is this needed - should be the same as osType(destination)
    ObjPtr destination;
} ObjPointer;

typedef struct ObjStruct {
    DynamicArray store; // of type KeyValue
} ObjStruct;

typedef struct ObjMap {
    DynamicArray entries; // of type KeyValue
} ObjMap;

typedef struct SyncElement {
    ObjPtr channel;
    ObjPtr result;
} SyncElement;

typedef struct ObjSyncGroup {
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

void printObject(ObjPtr);
void fprintObject(FILE *out, ObjPtr);

bool isObjOfType(ObjPtr obj, ObjType type);
bool isObjOfOneType(ObjPtr, size_t, ...);

bool isAddressValue(ObjPtr);

bool isArrayPointer(ObjPtr);
bool isStructPointer(ObjPtr);

#endif
