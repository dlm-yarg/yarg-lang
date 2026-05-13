//
//  dynamic_array.h
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//

#ifndef cyarg_dynamic_array_h
#define cyarg_dynamic_array_h

#include "object_type.h"

typedef struct Obj Obj;

typedef struct DynamicArray {
    ArrayItemCount arrayCapacity;
    ArrayItemCount arrayLength;
    ObjSize arrayItemSize;
    uint64_t arrayItems[0];
} DynamicArray;

void daInit(DynamicArray *, ObjSize);
void daExtend(Obj *enclosingObject, DynamicArray *, ArrayItemCount);
void daFree(DynamicArray *);

void daPushBack(DynamicArray *, void *);
void *daAt(DynamicArray *, ArrayItemCount);
ArrayItemCount daSize(DynamicArray *);

#endif
