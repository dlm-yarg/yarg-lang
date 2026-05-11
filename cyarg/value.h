#ifndef cyarg_value_h
#define cyarg_value_h

// Values are only used on the stack. They are variable sized
// Objects are used for globals and constants and may be referenced from (obj) values. Need objects for machine types.
// up values point to values on the stack.
// Object should carry their own type if required - struct, packed array.
// pointers  point to objects.

//before -- Debug,Address sanitiser,Stack after return, Undef behaviour,Main thread,Malloc scribble,Zombie
//gh.yarg-lang(44713,0x1fcbe2240) malloc: enabling scribbling to detect mods to free blocks
//int-perform benchmark: job size: 100, max job length: 10secs
//Generated 100 non-zero random numbers in 100 calls to int_rand()
//Add (x+y) time: 11secs, ops: 6612000, 601ops/ms
//Sub (x-y) time: 10secs, ops: 7000000, 700ops/ms
//Mul (x*y) time: 11secs, ops: 442200, 40ops/ms
//Div (x/y) time: 11secs, ops: 676200, 61ops/ms
//Mod (x%y) time: 11secs, ops: 795600, 72ops/ms
//Negate (-x) time: 10secs, ops: 7000000, 700ops/ms
//Program ended with exit code: 0





#include <string.h>
#include <stdio.h>

#include "common.h"
#include "big-int/big-int.h"

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#define IS_64BIT 1
#define IS_32BIT 0
#else
#define IS_64BIT 0
#define IS_32BIT 1
#endif

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjRoutine ObjRoutine;
typedef struct ObjConcreteYargType ObjConcreteYargType;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_DOUBLE,
    VAL_I8,
    VAL_UI8,
    VAL_I16,
    VAL_UI16,
    VAL_I32,
    VAL_UI32,
    VAL_UI64,
    VAL_I64,
    VAL_ADDRESS,
    VAL_INTCONCRETE,
    VAL_OBJ
} ValueType;

#if (VAL_OBJ > 3) // 255
#error run out of values
#endif

// AbstractValue when made concrete may be 4 to 1020 bytes long (only 512 needed for IntConcrete254)
struct AbstractValue {
    uint8_t type; // ValueType
    uint8_t numWords;   // 0 for nil;
                        // 1 for i8, i16, bool;
                        // 2 for i32 and address/obj in 32-bit builds;
                        // 3 for i64, double and address/obj in 64-bit builds; 3.. for int
    union {
        bool boolean;
        uint8_t ui8;
        int8_t i8;
        uint16_t ui16;
        int16_t i16;
    };
    union {
        uint32_t ui32;
        int32_t i32;
        IntConcrete2 intConcrete; // this may be larger
#if IS_32BIT
        uintptr_t address;
        Obj* obj;
#endif
    };
    union {
        double dbl;
        uint64_t ui64;
        int64_t i64;
#if IS_64BIT
        uintptr_t address;
        Obj* obj;
#endif
    };
};

struct ConcreteValue {
    uint8_t type;
    uint8_t numWords;
    int16_t placeHolder;
    IntConcrete254 placeHolder2;
};

typedef struct AbstractValue *Value;

#define IS_BOOL(value)     ((value)->type == VAL_BOOL)
#define IS_NIL(value)      ((value)->type == VAL_NIL)
#define IS_DOUBLE(value)   ((value)->type == VAL_DOUBLE)
#define IS_I8(value)       ((value)->type == VAL_I8)
#define IS_UI8(value)      ((value)->type == VAL_UI8)
#define IS_I16(value)      ((value)->type == VAL_I16)
#define IS_UI16(value)     ((value)->type == VAL_UI16)
#define IS_I32(value)      ((value)->type == VAL_I32)
#define IS_UI32(value)     ((value)->type == VAL_UI32)
#define IS_UI64(value)     ((value)->type == VAL_UI64)
#define IS_I64(value)      ((value)->type == VAL_I64)
#define IS_ADDRESS(value)  ((value)->type == VAL_ADDRESS)
#define IS_OBJ(value)      ((value)->type == VAL_OBJ)
#define IS_INT(value)      ((value)->type == VAL_OBJ && (value)->obj->type == OBJ_INT || \
                            (value)->type == VAL_INTCONCRETE)

#define AS_OBJ(value)      ((value)->obj)
#define AS_BOOL(value)     ((value)->boolean)
#define AS_I8(value)       ((value)->i8)
#define AS_UI8(value)      ((value)->ui8)
#define AS_I16(value)      ((value)->i16)
#define AS_UI16(value)     ((value)->ui16)
#define AS_I32(value)      ((value)->i32)
#define AS_UI32(value)     ((value)->ui32)
#define AS_UI64(value)     ((value)->ui64)
#define AS_I64(value)      ((value)->i64)
#define AS_ADDRESS(value)  ((value)->address)
#define AS_DOUBLE(value)   ((value)->dbl)

#define BOOL_VAL(to, value)     do {(to)->type = VAL_BOOL; (to)->boolean = (value); } while (0)
#define NIL_VAL(to)             do {(to)->type = VAL_NIL; /* (to)->i16 = 0; */ } while (0)
#define DOUBLE_VAL(to, value)   do {(to)->type = VAL_DOUBLE; (to)->dbl = (value); } while (0)
#define I8_VAL(to, value)       do {(to)->type = VAL_I8; (to)->i8 = (value); } while (0)
#define UI8_VAL(to, value)      do {(to)->type = VAL_UI8; (to)->ui8 = (value); } while (0)
#define I16_VAL(to, value)      do {(to)->type = VAL_I16; (to)->i16 = (value); } while (0)
#define UI16_VAL(to, value)     do {(to)->type = VAL_UI16; (to)->ui16 = (value); } while (0)
#define I32_VAL(to, value)      do {(to)->type = VAL_I32; (to)->i32 = (value); } while (0)
#define UI32_VAL(to, value)     do {(to)->type = VAL_UI32; (to)->ui32 = (value); } while (0)
#define I64_VAL(to, value)      do {(to)->type = VAL_I64; (to)->i64 = (value); } while (0)
#define UI64_VAL(to, value)     do {(to)->type = VAL_UI64; (to)->ui64 = (value); } while (0)
#define ADDRESS_VAL(to, a)      do {(to)->type = VAL_ADDRESS; (to)->address = (a); } while (0)
#define OBJ_VAL(to, object)     do {(to)->type = VAL_OBJ; (to)->obj = (Obj *)(object); } while (0)

#if IS_64BIT
#define SIZE_T_UI_VAL(to, value)   UI64_VAL((to), (value))
#elif IS_32BIT
#define SIZE_T_UI_VAL(to, value)   UI32_VAL((to), (value))
#endif

void CopyValue(Value to, Value const from);

bool is_positive_integer32(Value a);
uint32_t as_positive_integer32(Value a);

bool valuesEqual(Value a, Value b);

void printValue(Value value);
void fprintValue(FILE* op, Value value);

typedef union PackedValueStore PackedValueStore;

typedef struct {
    PackedValueStore* storedValue;
    ObjConcreteYargType* storedType;
} PackedValue;

void initialisePackedValue(PackedValue packedValue);
Value unpackValue(PackedValue packedValue);
PackedValue allocPackedValue(Value type);
void markPackedValue(PackedValue packedValue);

bool assignToPackedValue(PackedValue lhs, Value rhsValue);

bool is_uniformarray(PackedValue val);
bool is_struct(PackedValue val);
bool is_nil(PackedValue val);
bool is_channel(PackedValue val);

typedef struct {
    struct AbstractValue value;                 // this may need size reducing if used in collections or on stack
    ObjConcreteYargType* cellType;
} ValueCell;

typedef struct {
    Value value;
    ObjConcreteYargType* cellType;
} ValueCellTarget;

bool assignToValueCellTarget(ValueCellTarget lhs, Value rhsValue);
bool initialiseValueCellTarget(ValueCellTarget lhs, Value rhsValue);

typedef struct {
    int capacity;
    int count;
    Value* values;
} DynamicValueArray;

void initDynamicValueArray(DynamicValueArray* array);
void appendToDynamicValueArray(DynamicValueArray* array, Value value);
void freeDynamicValueArray(DynamicValueArray* array);

PackedValueStore* storedAddressof(Value value);

#endif
