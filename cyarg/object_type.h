//
//  object_type.h
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//

#ifndef cyarg_object_type_h
#define cyarg_object_type_h

#include <stdint.h>

enum /* ObjType */ {
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
    OBJ_STRING,

    OBJ_POINTER, // to any object including PLACOBJ_PTR_STRING_TYPE,ED
    OBJ_BLOB, // either object store or PLACED (ROM, DEVICE or BLOB); BLOBS do not move during osExtend() or osGc() so allocate early
    OBJ_UPVALUE,
    OBJ_ARRAY,
    OBJ_MAP,
    OBJ_STRUCT,
    // placed objects reference system memory - ROM, DEVICE or BLOB; may only comprise: integral, float, system addresses, other placed objects
    OBJ_PLACED, // OBJ_PLACED, OBJ_PLACED_ARRAY, OBJ_PLACED_STRUCT all use ObjPlaced
    OBJ_PLACED_ARRAY, // placed arrays have placeable elements
    OBJ_PLACED_STRUCT, // placed structs have placeable members
    OBJ_SYNCGROUP,
    OBJ_STACKSLICE,
    OBJ_YARGTYPE_ARRAY,
    OBJ_YARGTYPE_STRUCT,
    OBJ_YARGTYPE_MAP,

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
};

_Static_assert(OBJ_EXPR_TYPE_INDEXED_COLLECTION < OBJ_IN_ROM, "need to fix ObjType");

enum {
    // pre allocated objects
    OBJ_PTR_NIL,
    OBJ_PTR_TRUE,
    OBJ_PTR_FALSE,
    OBJ_PTR_NEGATIVE_ONE,
    OBJ_PTR_ZERO,
    OBJ_PTR_ONE,
    OBJ_PTR_TWO,
    OBJ_PTR_THREE,
    OBJ_PTR_FOUR,
    OBJ_PTR_EIGHT,
    OBJ_PTR_TEN,
    OBJ_PTR_TEN_THOUSAND,
    OBJ_PTR_THIS,

    // objects allocated on the fly
    OBJ_PTR_FIRST_OBJECT,

    OBJ_PTR_TYPE_TAGS = 0xff00, // 0xff00..0xfffe
    // OBJ_PTR_TYPE_TAGS are not ObjPtrs, they can’t be dereferenced, instead they are tags used in type descriptions
    OBJ_PTR_ANY_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_NIL,
    OBJ_PTR_BOOL_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_BOOL,
    OBJ_PTR_INT_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_INT,
    OBJ_PTR_ADDRESS_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_ADDRESS,
    OBJ_PTR_DOUBLE_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_DOUBLE,
    OBJ_PTR_I8_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_I8,
    OBJ_PTR_UI8_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_UI8,
    OBJ_PTR_I16_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_I16,
    OBJ_PTR_UI16_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_UI16,
    OBJ_PTR_I32_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_I32,
    OBJ_PTR_UI32_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_UI32,
    OBJ_PTR_I64_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_I64,
    OBJ_PTR_UI64_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_UI64,
    OBJ_PTR_BOUND_METHOD_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_BOUND_METHOD,
    OBJ_PTR_CLASS_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_CLASS,
    OBJ_PTR_CLOSURE_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_CLOSURE,
    OBJ_PTR_FUNCTION_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_FUNCTION,
    OBJ_PTR_INSTANCE_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_INSTANCE,
    OBJ_PTR_NATIVE_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_NATIVE,
    OBJ_PTR_ROUTINE_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_ROUTINE,
    OBJ_PTR_CHANNELCONTAINER_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_CHANNELCONTAINER,
    OBJ_PTR_STRING_TYPE = OBJ_PTR_TYPE_TAGS + OBJ_STRING,
    OBJ_PTR_TYPE_TAGS_END
};

_Static_assert(OBJ_PTR_TYPE_TAGS_END <= 0xfffe, "need to fix ObjPtr type tags");

typedef uint8_t ObjType;
typedef uint16_t ObjPtr;
typedef uint16_t ObjSize;
typedef uint16_t ArrayItemCount;
typedef uint16_t ElementOffset;

// for maps/structs/classes
typedef struct KeyValue {
    ObjPtr key;
    ObjPtr value;
} KeyValue;

// for placed-structs
typedef struct KeyOffsetType {
    ObjPtr key;
    ElementOffset offset;
    ObjPtr type;
} KeyOffsetType;

typedef struct Obj Obj;

#endif
