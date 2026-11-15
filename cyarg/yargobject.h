#ifndef cyarg_yargobject_h
#define cyarg_yargobject_h

#include "object.h"
#include "big-int/big-int.h"
#include "value.h"
#include "chunk.h"
#include "table.h"
#include "keyvaluestore.h"

typedef struct ObjConcreteYargType ObjConcreteYargType;

#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_ROUTINE(value)      isObjType(value, OBJ_ROUTINE)
#define IS_CHANNEL(value)      isObjType(value, OBJ_CHANNELCONTAINER)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_YARGTYPE(value)     (isObjType(value, OBJ_YARGTYPE) || isObjType(value, OBJ_YARGTYPE_ARRAY) || isObjType(value, OBJ_YARGTYPE_STRUCT) || isObjType(value, OBJ_YARGTYPE_POINTER) || isObjType(value, OBJ_YARGTYPE_MAP))
#define IS_SYNCGROUP(value)    isObjType(value, OBJ_SYNCGROUP)
#define IS_MAP(value)          isObjType(value, OBJ_MAP)

#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_ROUTINE(value)      ((ObjRoutine*)AS_OBJ(value))
#define AS_CHANNEL(value)      ((ObjChannelContainer*)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define AS_SYNCGROUP(value)    ((ObjSyncGroup*)AS_OBJ(value))
#define AS_INTOBJ(value)       (assert((value).as.bigSmallInt.obj_), (value).as.bigSmallInt.objInt_)
#define AS_MAP(value)          ((ObjMap*)AS_OBJ(value))

#define STRING_VAL(object)     ((Value){.type = &yargTypes.string, .as.obj = (Obj*) object})
#define INTOBJ_VAL(object)     ((Value){.type = &yargTypes.integer, .as.bigSmallInt.obj_ = true, .as.bigSmallInt.objInt_ = object})
#define CLASS_VAL(object)      ((Value){.type = &yargTypes.class_, .as.obj = (Obj*) object})
#define INSTANCE_VAL(object)   ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define FUNCTION_VAL(object)   ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define CLOSURE_VAL(object)    ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define ROUTINE_VAL(object)    ((Value){.type = &yargTypes.routine, .as.obj = (Obj*) object})
#define NATIVE_VAL(object)     ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define CHANNEL_VAL(object)    ((Value){.type = &yargTypes.channel, .as.obj = (Obj*) object})
#define SYNCGROUP_VAL(object)  ((Value){.type = &yargTypes.syncGroup, .as.obj = (Obj*) object})
#define BOUNDMETHOD_VAL(object)  ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define MAP_VAL(object)        ((Value){.type = ((const ObjConcreteYargType*)((ObjMap*)object)->type), .as.obj = (Obj*) object})

Value cstring_val(const char*);

struct ObjString {
    Obj obj;
    char *chars; // vm_alloc’d
    uint32_t length;
#if defined CYARG_ORDERED_STRING_TREE
    struct {
        struct ObjString *t; // toward root or zero if root
        struct ObjString *l; // prior
        struct ObjString *r; // subsequent
        int16_t n; // distance to leaf: 0 if leaf, max(l->n, r->n) + 1 otherwise
        uint16_t p : 1; // 0 if parsing/ed left, 1 if parsing/ed right
    } sss;
#else
    uint32_t hash; // for use in maps
#endif
};

typedef struct ObjInt {
    Obj obj;
    Int bigInt;
} ObjInt;

typedef struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* fName;
} ObjFunction;

typedef struct {
    Obj obj;
    ObjString* name;
#if defined CYARG_ORDERED_STRING_TREE
    KeyValueStore methods;
#else
    ValueTable methods;
#endif
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass* klass;
#if defined CYARG_ORDERED_STRING_TREE
    KeyValueStore fields;
#else
    ValueTable fields;
#endif
} ObjInstance;

typedef struct {
    Obj obj;
    ObjConcreteYargTypeMap* type;
#if defined CYARG_ORDERED_STRING_TREE
    KeyValueStore entries;
#else
    ValueTable entries;
#endif
} ObjMap;

ObjFunction* newFunction();
void initFunction(ObjFunction* function);
ObjString* functionToString(const ObjFunction* function);

ObjClass* newClass(ObjString* name);
ObjInstance* newInstance(ObjClass* klass);

ObjMap* newMap(ObjConcreteYargTypeMap* type);
ObjString* mapToString(const ObjMap* map);

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjString* copyStringWithEscapes(const char* chars, int length);
ObjString* concatenateStrings(ObjString* a, ObjString* b);

ObjInt* allocateIntObject(size_t numDigits);
Value defaultIntValue();
void noLongerLiteralInt(Value *value);

#endif
