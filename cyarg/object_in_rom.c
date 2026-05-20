//
//  object_in_rom.c
//  gh.yarg-lang
//
//  Created by dlm on 18/05/2026.
//

#include "object_in_rom.h"
#include "yargtype.h"

ObjNil const oirNil = { .obj = { .objType = OBJ_IN_ROM + OBJ_NIL } };

ObjBool const oirTrue = { .obj = { .objType = OBJ_IN_ROM + OBJ_BOOL }, .b = true };
ObjBool const oirFalse = { .obj = { .objType = OBJ_IN_ROM + OBJ_BOOL }, .b = false };

ObjInt2 const oirNegativeOne = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = true, .overflow_ = false, .w_ = 1 }, .isLiteral = true };
ObjInt2 const oirZero = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 0 }, .isLiteral = true };
ObjInt2 const oirOne = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 1 }, .isLiteral = true };
ObjInt2 const oirTwo = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 2 }, .isLiteral = true };
ObjInt2 const oirThree = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 3 }, .isLiteral = true };
ObjInt2 const oirFour = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 4 }, .isLiteral = true };
ObjInt2 const oirEight = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 8 }, .isLiteral = true };
ObjInt2 const oirTen = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 10 }, .isLiteral = true };
ObjInt2 const oirTenThousand = { .obj = { .objType = OBJ_IN_ROM + OBJ_INT }, .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 10000 }, .isLiteral = true };

ObjString7 const oirThis = { .obj = { .objType = OBJ_IN_ROM + OBJ_STRING }, .s = { .arrayCapacity = 8, .arrayLength = 4 , .arrayItemSize = 1, .chars = "this" } };

ObjConcreteYargType const oirYargTypeAny = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_NIL };
ObjConcreteYargType const oirYargTypeBool = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_BOOL };
ObjConcreteYargType const oirYargTypeInt = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_INT };
ObjConcreteYargType const oirYargTypeDouble = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_DOUBLE };
ObjConcreteYargType const oirYargTypeInt8 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_I8 };
ObjConcreteYargType const oirYargTypeUint8 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_UI8 };
ObjConcreteYargType const oirYargTypeInt16 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_I16 };
ObjConcreteYargType const oirYargTypeUint16 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_UI16 };
ObjConcreteYargType const oirYargTypeInt32 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_I32 };
ObjConcreteYargType const oirYargTypeUint32 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_UI32 };
ObjConcreteYargType const oirYargTypeInt64 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_I64 };
ObjConcreteYargType const oirYargTypeUint64 = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_UI64 };
ObjConcreteYargType const oirYargTypeString = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_STRING };
ObjConcreteYargType const oirYargTypeClass = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_CLASS };
ObjConcreteYargType const oirYargTypeInstance = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_INSTANCE };
ObjConcreteYargType const oirYargTypeFunction = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_FUNCTION };
ObjConcreteYargType const oirYargTypeRoutine = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_ROUTINE };
ObjConcreteYargType const oirYargTypeChannel = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_CHANNELCONTAINER };
ObjConcreteYargType const oirYargTypeYargType = { .obj = { .objType = OBJ_IN_ROM + OBJ_YARGTYPE }, .yt = OBJ_YARGTYPE };
