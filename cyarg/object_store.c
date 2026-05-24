//
//  object_store.c
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//

#include "object_store.h"
#include "dynamic_array.h"
#include "object_in_rom.h"
#include "yargtype.h"

#include <stdlib.h>
#include <assert.h>

#define HASH_MAX 256

enum {
    OS_FLAG_COPIED = 0x0001u,
    OS_FLAG_LAZY = 0x0002u
};

// note - it is more memory efficient to have a seperate array for each member of PtrEntry, or to stuff this into Obj
// but the code is simpler to do it as struct, and debugging is nicer to not have os internals polluting the clients view
typedef struct PtrEntry {
    Obj const *cell;
    uint32_t size; // this can be stored in flags once alloc pools are implemented
    ObjPtr copy; // this should be ignored unless OS_FLAG_COPY is set
    uint16_t flags;
} PtrEntry;

typedef struct PtrArray {
    ArrayItemCount arrayCapacity;
    ArrayItemCount arrayLength;
    PtrEntry arrayItems[0];
} PtrArray;

static PtrArray *osRamPtrs;
static PtrArray *osRomPtrs;
static ObjPtr stringByHash[HASH_MAX];

// these bracket calls to alloc, owned, free, copy, modify and gc
static void lock(void);
static void unlock(void);

void osInit(void) {
    // create pools - for now we just use malloc

    // create Ptr tables - for now don’t grow
    osRamPtrs = malloc(sizeof (DynamicArray) + sizeof (PtrEntry) * 200);
    osRamPtrs->arrayCapacity = 200;
    osRamPtrs->arrayLength = 0;

    osRomPtrs = malloc(sizeof (DynamicArray) + sizeof (PtrEntry) * 100);
    osRomPtrs->arrayCapacity = 100;
    osRomPtrs->arrayLength = 0;

    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirNil.obj, .size = sizeof oirNil, .copy = OS_NOT_COPIED, .flags = 0 };

    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirTrue.obj, .size = sizeof oirTrue, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirFalse.obj, .size = sizeof oirFalse, .copy = OS_NOT_COPIED, .flags = 0 };

    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirNegativeOne.obj, .size = sizeof oirNegativeOne, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirZero.obj, .size = sizeof oirZero, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirOne.obj, .size = sizeof oirOne, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirTwo.obj, .size = sizeof oirTwo, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirThree.obj, .size = sizeof oirThree, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirFour.obj, .size = sizeof oirFour, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirEight.obj, .size = sizeof oirEight, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirTen.obj, .size = sizeof oirTen, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirTenThousand.obj, .size = sizeof oirTenThousand, .copy = OS_NOT_COPIED, .flags = 0 };

    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirThis.obj, .size = sizeof oirThis, .copy = OS_NOT_COPIED, .flags = 0 };

    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeAny.obj, .size = sizeof oirYargTypeAny, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeBool.obj, .size = sizeof oirYargTypeBool, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeInt.obj, .size = sizeof oirYargTypeInt, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeDouble.obj, .size = sizeof oirYargTypeDouble, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeInt8.obj, .size = sizeof oirYargTypeInt8, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeUint8.obj, .size = sizeof oirYargTypeUint8, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeInt16.obj, .size = sizeof oirYargTypeInt16, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeUint16.obj, .size = sizeof oirYargTypeUint16, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeInt32.obj, .size = sizeof oirYargTypeInt32, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeUint32.obj, .size = sizeof oirYargTypeUint32, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeInt64.obj, .size = sizeof oirYargTypeInt64, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeUint64.obj, .size = sizeof oirYargTypeUint64, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeString.obj, .size = sizeof oirYargTypeString, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeClass.obj, .size = sizeof oirYargTypeClass, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeInstance.obj, .size = sizeof oirYargTypeInstance, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeFunction.obj, .size = sizeof oirYargTypeFunction, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeRoutine.obj, .size = sizeof oirYargTypeRoutine, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeChannel.obj, .size = sizeof oirYargTypeChannel, .copy = OS_NOT_COPIED, .flags = 0 };
    osRomPtrs->arrayItems[osRomPtrs->arrayLength++] = (PtrEntry){ .cell = &oirYargTypeYargType.obj, .size = sizeof oirYargTypeYargType, .copy = OS_NOT_COPIED, .flags = 0 };
}

void osExtend(void) {}

ObjPtr osAlloc(ObjSize sz) {
    lock();
    ArrayItemCount newP = osRamPtrs->arrayLength++;
    osRamPtrs->arrayItems[newP] = (PtrEntry){ .cell = malloc(sz), .copy = OS_NOT_COPIED, .flags = 0 };
    unlock();
    return newP;
}

void osNoGc(ObjPtr p) {
    lock();
    unlock();
}

void osGcOk(ObjPtr p) {
    lock();
    unlock();
}

void osRealloc(ObjPtr p, ObjSize sz) {
    lock();
    unlock();
}


static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tempRootPush((Obj *)(string));
    struct AbstractValue v;
    NIL_VAL(&v);
    tableSet(&vm.strings, string, &v);
    tempRootPop();
    return string;
}

static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash += 16777619;
    }
    return hash;
}

ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}


ObjPtr osStoreString(char *s) {
    
}

void osFree(ObjPtr p) {
    lock();
    unlock();
}

ObjPtr osCopy(ObjPtr p) {
    lock();
    assert(p >= OIR_NIL && p - OIR_NIL < osRomPtrs->arrayLength || p < OIR_NIL && osRamPtrs->arrayLength);
    PtrEntry *from = p >= OIR_NIL ? &osRomPtrs->arrayItems[p - OIR_NIL] : &osRamPtrs->arrayItems[p];
    ArrayItemCount newP = osRamPtrs->arrayLength++;
    PtrEntry *to = &osRamPtrs->arrayItems[newP];
    if ((from->flags & (OS_FLAG_COPIED | OS_FLAG_LAZY)) == 0) { // not a lazy copy or copied
        *to = (PtrEntry){ .cell = 0, .copy = p, .flags = OS_FLAG_LAZY };
        from->flags |= OS_FLAG_COPIED;
        from->copy = newP;
    } else {
        *to = (PtrEntry){ .cell = malloc(from->size), .copy = OS_NOT_COPIED, .flags = 0 };
        memcpy(&to->cell, &from->cell, from->size);
    }
    unlock();
    return newP;
}

ObjPtr osStoreRomObject(Obj *obj) {return 0;}

void osGc(void) {}

Obj const *osDeref(ObjPtr p) {
    if (p >= OIR_NIL) {
        assert(p - OIR_NIL < osRomPtrs->arrayLength);
        return osRomPtrs->arrayItems[p - OIR_NIL].cell;
    } else {
        assert(p < osRamPtrs->arrayLength);
        return osRamPtrs->arrayItems[p].cell;
    }
}

Obj *osDerefAndModify(ObjPtr p) {
    lock();
    assert(p < osRamPtrs->arrayLength);
    PtrEntry *pe = &osRamPtrs->arrayItems[p];
    if ((pe->flags & (OS_FLAG_COPIED | OS_FLAG_LAZY)) == 0) { // !copied && !lazy - just use
        assert(pe->copy == OS_NOT_COPIED);
        unlock();
    } else { // copied || lazy - copy copied to lazy
        assert(pe->copy != OS_NOT_COPIED);
        ObjPtr copyOrCopiedPtr = pe->copy;
        PtrEntry *copyOrCopied = copyOrCopiedPtr >= OIR_NIL ? &osRomPtrs->arrayItems[copyOrCopiedPtr - OIR_NIL] : &osRamPtrs->arrayItems[copyOrCopiedPtr];
        PtrEntry *from, *to;
        if ((copyOrCopied->flags & (OS_FLAG_COPIED | OS_FLAG_LAZY)) == OS_FLAG_LAZY) {
            assert((pe->flags & (OS_FLAG_COPIED | OS_FLAG_LAZY)) == OS_FLAG_COPIED);
            to = copyOrCopied;
            from = pe;
        } else {
            assert((copyOrCopied->flags & (OS_FLAG_COPIED | OS_FLAG_LAZY)) == OS_FLAG_COPIED &&
                   (pe->flags & (OS_FLAG_COPIED | OS_FLAG_LAZY)) == OS_FLAG_LAZY);
            to = pe;
            from = copyOrCopied;
        }
        to->copy = from->copy = OS_NOT_COPIED;
        to->flags &= ~(OS_FLAG_COPIED | OS_FLAG_LAZY);
        from->flags &= ~(OS_FLAG_COPIED | OS_FLAG_LAZY);
        to->cell = malloc(from->size);
        to->size = from->size;
        unlock();
        memcpy(&to->cell, &from->cell, from->size);
    }
    return (Obj *)pe->cell;
}

void lock(void) {}

void unlock(void) {}
