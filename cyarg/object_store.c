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
#include <string.h>
#include <assert.h>

enum {
    OS_FLAG_COPIED = 0x0001u,
    OS_FLAG_COPY = 0x0002u
};

// note - it is more memory efficient to have a seperate array for each member of PtrEntry, or to stuff this into Obj
// but the code is simpler to do it as struct, and debugging is nicer to not have os internals polluting the clients view
typedef struct PtrEntry {
    union {
        Obj const *cell; // usually unique in osR?mPtrs, if a copy (flag set) the obj is read-only if written (osDerefAndModify) obj is duplicated and and copy flags are reset
        struct PtrEntry *nextFreeEntry;
    };
    uint32_t size; // this can be stored in flags/copy once pools are implemented; not needed for ROM objects
    ObjPtr copy;
    uint16_t flags;
} PtrEntry;

typedef struct HashTreeElement {
    uint16_t hash;
    ObjPtr s;
    ObjPtr sameHash;
    ObjPtr higherHash;
    ObjPtr lowerHash;
} HashTreeElement;

#define NUM_PTR_ENTRIES 2048
PtrEntry ptrEntries[NUM_PTR_ENTRIES];
PtrEntry *freePtrEntries = 0;

#define NUM_HASH_ENTRIES 256
static ObjPtr hashTable[NUM_HASH_ENTRIES];

static void returnPtrEntry(PtrEntry *);
static PtrEntry *allocPtrEntry(void);

// these bracket calls to alloc, owned, free, copy, modify and gc
static void lock(void);
static void unlock(void);

// string support
static uint8_t hashString(char const *, ArrayItemCount);
static ObjPtr storeString(ObjPtr);
static void deleteString(ObjPtr); // called from GC

void osInit(void) {
    // create pools - for now we just use malloc

    // create prt entry free list - remove and add from start
    for (int i = 0; i < NUM_PTR_ENTRIES; i++) {
        returnPtrEntry(&ptrEntries[i]);
    }

    PtrEntry *newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirNil.obj, .size = sizeof oirNil, .copy = 0, .flags = 0 };
    assert(newPe - ptrEntries == OBJ_PTR_NIL);

    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirTrue.obj, .size = sizeof oirTrue, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirFalse.obj, .size = sizeof oirFalse, .copy = 0, .flags = 0 };

    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirNegativeOne.obj, .size = sizeof oirNegativeOne, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirZero.obj, .size = sizeof oirZero, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirOne.obj, .size = sizeof oirOne, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirTwo.obj, .size = sizeof oirTwo, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirThree.obj, .size = sizeof oirThree, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirFour.obj, .size = sizeof oirFour, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirEight.obj, .size = sizeof oirEight, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirTen.obj, .size = sizeof oirTen, .copy = 0, .flags = 0 };
    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirTenThousand.obj, .size = sizeof oirTenThousand, .copy = 0, .flags = 0 };

    newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = &oirThis.obj, .size = sizeof oirThis, .copy = 0, .flags = 0 };
    assert(newPe - ptrEntries == OBJ_PTR_THIS);
    storeString(OBJ_PTR_THIS);
}

void osExtend(void) {}

ObjPtr osAlloc(ObjSize sz) {
    lock();
    PtrEntry *newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = malloc(sz), .copy = 0, .flags = 0 };
    unlock();
    return newPe - ptrEntries;
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

void osFree(ObjPtr p) {
    lock();
    unlock();
}

ObjPtr osCopy(ObjPtr p) {
    lock();
    PtrEntry *from = &ptrEntries[p];
    PtrEntry *newPe = allocPtrEntry();
    if ((from->flags & (OS_FLAG_COPIED | OS_FLAG_COPY)) == 0) { // for now only alow one copy, however PtrEntry::copy, could be a list and allow multiple copies
        *newPe = (PtrEntry){ .cell = from->cell, .copy = p, .flags = OS_FLAG_COPY };
        from->flags |= OS_FLAG_COPIED;
        from->copy = newPe - ptrEntries;
    } else {
        *newPe = (PtrEntry){ .cell = malloc(from->size), .size = from->size, .copy = 0, .flags = 0 };
        memcpy(&newPe->cell, &from->cell, from->size);
    }
    unlock();
    return newPe - ptrEntries;
}

ObjPtr osStoreRomObject(Obj *obj) {
    lock();
    PtrEntry *newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .cell = obj, .size = 0, .copy = 0, .flags = 0 };
    unlock();
    return newPe - ptrEntries;
}

void osGc(void) {}

void const *osDeref(ObjPtr p) {
    assert(p < NUM_PTR_ENTRIES);
    return ptrEntries[p].cell;
}

Obj *osDerefAndModify(ObjPtr p) {
    lock();
    assert(p < NUM_PTR_ENTRIES);
    PtrEntry *pe = &ptrEntries[p];
    if ((pe->flags & (OS_FLAG_COPIED | OS_FLAG_COPY)) == 0) { // !copied && !lazy - just use
        unlock();
    } else { // copied || lazy - copy copied to lazy
        ObjPtr copyOrCopiedPtr = pe->copy;
        PtrEntry *copyOrCopied = &ptrEntries[copyOrCopiedPtr];
        assert(copyOrCopied->copy == p);
        assert(pe->size == copyOrCopied->size);

        copyOrCopied->copy = pe->copy = 0; // debug
        copyOrCopied->flags &= ~(OS_FLAG_COPIED | OS_FLAG_COPY);
        pe->flags &= ~(OS_FLAG_COPIED | OS_FLAG_COPY);

        pe->cell = malloc(copyOrCopied->size);
        unlock();

        memcpy(&pe->cell, &copyOrCopied->cell, copyOrCopied->size);
    }
    return (Obj *)pe->cell; // const cast
}

//static ObjString* allocateString(char const *chars, int length) {
//    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
//    string->length = length;
//    string->chars = chars;
//    string->hash = hash;
//    tempRootPush((Obj *)(string));
//    struct AbstractValue v;
//    NIL_VAL(&v);
//    tableSet(&vm.strings, string, &v);
//    tempRootPop();
//    return string;
//}


ObjPtr osStoreCString(char const *s, ArrayItemCount l) {
    ObjSize size = sizeof (ObjString) + sizeof (uint64_t) * ((l + 7) / 8);

    PtrEntry *newPe = allocPtrEntry();
    ObjPtr newP = newPe - ptrEntries;
    *newPe = (PtrEntry){ .cell = malloc(size), .size = size, .copy = 0, .flags = 0 }; // is size correct or is it

    ObjString *so = (ObjString *)newPe->cell;
    *so = (ObjString){ .sameHash = OBJ_PTR_NIL, .sLength = l, .sCapacity = size - sizeof (ObjString) };
    so->obj.objType = OBJ_STRING;
    memcpy(so->chars, s, l);
    return storeString(newP);
}

ObjPtr osStoreRomCString(char const *s) {
    lock();
    PtrEntry *newPe = allocPtrEntry();
    ObjPtr newP = newPe - ptrEntries;
    *newPe = (PtrEntry){ .cell = malloc(sizeof (ObjRomString)), .copy = 0, .flags = 0 };
    unlock();

    ObjRomString *so = (ObjRomString *)newPe->cell;
    so->obj.objType = OBJ_STRING | OBJ_IN_ROM;
    so->cStr = s;
    so->sameHash = OBJ_PTR_NIL;
    return storeString(newP);
}

char const *osStringAsCString(ObjPtr p) {
    ObjString const *s = osDeref(p);
    assert((s->obj.objType & ~OBJ_IN_ROM) == OBJ_STRING);
    if (s->sLength < s->sCapacity) {
        return s->chars;
    } else { // if (sLength == sCapacity) {
        return osStringAsCString(osStoreCString(s->chars, s->sLength));
    }
}

void returnPtrEntry(PtrEntry *pe) {
    pe->nextFreeEntry = freePtrEntries;
    freePtrEntries = pe;
}

PtrEntry *allocPtrEntry(void) {
    assert(freePtrEntries != 0);
    PtrEntry *r = freePtrEntries;
    freePtrEntries = r->nextFreeEntry;
    r->nextFreeEntry = (PtrEntry *)1; // debug only
    return r;
}

void lock(void) {}

void unlock(void) {}

static uint8_t hashString(char const *s, ArrayItemCount l) {
    uint8_t hash = 0;
    char const *b = &s[0];
    char const *e = &s[l - 1];
    while (b < e) {
        hash = (hash << 5) | (hash >> (8 - 5));
        hash ^= (uint8_t)*b++;
    }
    return hash;
}

static ObjPtr storeString(ObjPtr p) {
    osNoGc(p);
    Obj const *s = osDeref(p);
    char const *sa;
    ArrayItemCount sl;
    if ((s->objType & OBJ_IN_ROM) != 0) {
        assert(s->objType == (OBJ_STRING | OBJ_IN_ROM));
        sa = ((ObjRomString const *)s)->cStr;
        sl = strlen(sa);
    } else {
        assert(s->objType == OBJ_STRING);
        sa = ((ObjString const *)s)->chars;
        sl = ((ObjString const *)s)->sLength;
    }
    uint8_t hash = hashString(sa, sl);

    // see if exists
    ObjPtr hashHead = hashTable[hash];
    bool found = false;
    while (!found && hashHead != 0) {
        char const *ta;
        ArrayItemCount tl;
        ObjString const *t = osDeref(hashHead);
        osNoGc(hashHead);
        if (t->obj.objType == (OBJ_IN_ROM | OBJ_STRING)) {
            ta = ((ObjRomString const *)s)->cStr;
            tl = strlen(sa);
        } else {
            assert(s->objType == OBJ_STRING);
            ta = ((ObjString const *)s)->chars;
            tl = ((ObjString const *)s)->sLength;
        }
        if (sl == tl && memcmp(sa, ta, sl) == 0) {
            found = true;
        }
        osGcOk(hashHead);
        hashHead = t->sameHash;
    }

    if (!found) {
        osGcOk(p);
        ((ObjString *)s)->sameHash = hashTable[hash]; // sameHash is volatile
        hashTable[hash] = p;
    } else {
        osGcOk(p);
        p = hashHead;
    }

    return p;
}

static void deleteString(ObjPtr p) {
    osNoGc(p);
    Obj const *s = osDeref(p);
    char const *sa;
    ArrayItemCount sl;
    if ((s->objType & OBJ_IN_ROM) != 0) { // todo - should rom strings be retained?
        assert(s->objType == (OBJ_STRING | OBJ_IN_ROM));
        sa = ((ObjRomString const *)s)->cStr;
        sl = strlen(sa);
    } else {
        assert(s->objType == OBJ_STRING);
        sa = ((ObjString const *)s)->chars;
        sl = ((ObjString const *)s)->sLength;
    }
    uint8_t hash = hashString(sa, sl);

    ObjPtr *prev = &hashTable[hash];
    ObjPtr hashHead = hashTable[hash];
    bool found = false;
    while (!found && hashHead != 0) {
        char const *ta;
        ArrayItemCount tl;
        ObjString const *t = osDeref(hashHead);
        osNoGc(hashHead);
        if (t->obj.objType == (OBJ_IN_ROM | OBJ_STRING)) {
            ta = ((ObjRomString const *)s)->cStr;
            tl = strlen(sa);
        } else {
            assert(s->objType == OBJ_STRING);
            ta = ((ObjString const *)s)->chars;
            tl = ((ObjString const *)s)->sLength;
        }
        if (sl == tl && memcmp(sa, ta, sl) == 0) {
            *prev = t->sameHash;
            found = true;
        }
        osGcOk(hashHead);
        prev = (ObjPtr *)&t->sameHash; // sameHash is volatile
        hashHead = t->sameHash;
    }

    osGcOk(p);
    osFree(p);
}
