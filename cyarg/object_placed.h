//
//  object_placed.h
//  gh.yarg-lang
//
//  Created by dlm on 18/08/2026.
//

#include "object_type.h"
#include <stdlib.h>

void opStore(ObjPtr from, uintptr_t to);
void opLoad(uintptr_t from, ObjPtr to);
void opCopy(uintptr_t from, uintptr_t to, ObjPtr type);
