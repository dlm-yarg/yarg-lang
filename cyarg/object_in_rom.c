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

ObjRomString const oirThis = { .obj = { .objType = OBJ_IN_ROM + OBJ_STRING }, .cStr = "this" };
