//
//  object_in_rom.h
//  gh.yarg-lang
//
//  Created by dlm on 18/05/2026.
//

#include "object.h"

// can access these objects directly by the following names rathere than derefrencing OBJ_PTR_NIL etc.
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

extern ObjRomString const oirThis;
