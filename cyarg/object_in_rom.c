//
//  object_in_rom.c
//  gh.yarg-lang
//
//  Created by dlm on 18/05/2026.
//

#include "object_in_rom.h"
#include "yargtype.h"

ObjBool const oirTrue = { .b = true };
ObjBool const oirFalse = { .b = false };

ObjInt2 const oirNegativeOne = { .i = { .d_ = 1, .m_ = 2, .neg_ = true, .overflow_ = false, .w_ = 1 }, .isLiteral = true };
ObjInt2 const oirZero = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 0 }, .isLiteral = true };
ObjInt2 const oirOne = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 1 }, .isLiteral = true };
ObjInt2 const oirTwo = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 2 }, .isLiteral = true };
ObjInt2 const oirThree = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 3 }, .isLiteral = true };
ObjInt2 const oirFour = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 4 }, .isLiteral = true };
ObjInt2 const oirEight = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 8 }, .isLiteral = true };
ObjInt2 const oirTen = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 10 }, .isLiteral = true };
ObjInt2 const oirTenThousand = { .i = { .d_ = 1, .m_ = 2, .neg_ = false, .overflow_ = false, .w_ = 10000 }, .isLiteral = true };

ObjRomString const oirThis = { .cStr = "this" };

PtrEntry const romPtrEntries[] = {
    { .obj = (void *)&oirTrue, .sz = sizeof (ObjBool), .copyOf = 0, .objType = OBJ_BOOL | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirFalse, .sz = sizeof (ObjBool), .copyOf = 0, .objType = OBJ_BOOL | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirNegativeOne, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirZero, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirOne, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirTwo, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirThree, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirFour, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirEight, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirTen, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirTenThousand, .sz = sizeof (ObjInt2), .copyOf = 0, .objType = OBJ_INT | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
    { .obj = (void *)&oirThis, .sz = sizeof (ObjRomString), .copyOf = 0, .objType = OBJ_STRING | OBJ_IN_ROM, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = true },
};
