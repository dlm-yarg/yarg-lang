//
//  object_in_rom.h
//  gh.yarg-lang
//
//  Created by dlm on 18/05/2026.
//

#include "object.h"

enum {
    OIR_NIL = 49152,

    OIR_TRUE,
    OIR_FALSE,

    OIR_NEGATIVE_ONE,
    OIR_ZERO,
    OIR_ONE,
    OIR_TWO,
    OIR_THREE,
    OIR_FOUR,
    OIR_EIGHT,
    OIR_TEN,
    OIR_TEN_THOUSAND,

    OIR_THIS,

    OIR_YT_ANY,
    OIR_YT_BOOL,
    OIR_YT_INT,
    OIR_YT_DOUBLE,
    OIR_YT_INT8,
    OIR_YT_UINT8,
    OIR_YT_INT16,
    OIR_YT_UINT16,
    OIR_YT_INT32,
    OIR_YT_UINT32,
    OIR_YT_INT64,
    OIR_YT_UINT64,
    OIR_YT_STRING,
    OIR_YT_CLASS,
    OIR_YT_INSTANCE,
    OIR_YT_FUNCTION,
    OIR_YT_ROUTINE,
    OIR_YT_CHANNEL,
    OIR_YT_YARGTYPE,

    OS_NOT_COPIED = 65535
};

_Static_assert(OIR_YT_ANY < OS_NOT_COPIED, "run out of object names -- make objTyp uint16_t");

extern ObjNil const oirNil;

extern ObjBool const oirTrue;
extern ObjBool const oirFalse;

extern ObjInt2 const oirNegativeOne;
extern ObjInt2 const oirZero;
extern ObjInt2 const oirOne;
extern ObjInt2 const oirTwo;
extern ObjInt2 const oirThree;
extern ObjInt2 const oirFour;
extern ObjInt2 const oirEight;
extern ObjInt2 const oirTen;
extern ObjInt2 const oirTenThousand;

extern ObjString7 const oirThis;

typedef struct ObjConcreteYargType ObjConcreteYargType;

extern ObjConcreteYargType const oirYargTypeAny;
extern ObjConcreteYargType const oirYargTypeBool;
extern ObjConcreteYargType const oirYargTypeInt;
extern ObjConcreteYargType const oirYargTypeDouble;
extern ObjConcreteYargType const oirYargTypeInt8;
extern ObjConcreteYargType const oirYargTypeUint8;
extern ObjConcreteYargType const oirYargTypeInt16;
extern ObjConcreteYargType const oirYargTypeUint16;
extern ObjConcreteYargType const oirYargTypeInt32;
extern ObjConcreteYargType const oirYargTypeUint32;
extern ObjConcreteYargType const oirYargTypeInt64;
extern ObjConcreteYargType const oirYargTypeUint64;
extern ObjConcreteYargType const oirYargTypeString;
extern ObjConcreteYargType const oirYargTypeClass;
extern ObjConcreteYargType const oirYargTypeInstance;
extern ObjConcreteYargType const oirYargTypeFunction;
extern ObjConcreteYargType const oirYargTypeRoutine;
extern ObjConcreteYargType const oirYargTypeChannel;
extern ObjConcreteYargType const oirYargTypeYargType;
