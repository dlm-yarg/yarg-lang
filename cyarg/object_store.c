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

#define NUM_POOLS 7

#ifdef RP2350
#define POOL16_LEN 1382
#define POOL64_LEN 461
#define POOL256_LEN 154
#define POOL1024_LEN 52
#define POOL4096_LEN 18
#define POOL16384_LEN 6
#define POOL65536_LEN 2
#define NUM_PTR_ENTRIES 2490
#else // RP2040
#define POOL16_LEN 634
#define POOL64_LEN 212
#define POOL256_LEN 71
#define POOL1024_LEN 24
#define POOL4096_LEN 8
#define POOL16384_LEN 3
#define POOL65536_LEN 1
#define NUM_PTR_ENTRIES 1144
#endif

typedef struct HashTreeElement {
    uint16_t hash;
    ObjPtr s;
    ObjPtr sameHash;
    ObjPtr higherHash;
    ObjPtr lowerHash;
} HashTreeElement;

PtrEntry ptrEntries[NUM_PTR_ENTRIES];
PtrEntry *freePtrEntries = 0; // alloc ptrEntries from and return to head of list
uint64_t pool16[POOL16_LEN][2];
uint64_t pool64[POOL64_LEN][8];
uint64_t pool256[POOL256_LEN][32];
uint64_t pool1024[POOL1024_LEN][128];
uint64_t pool4096[POOL4096_LEN][512];
uint64_t pool16384[POOL16384_LEN][2048];
uint64_t pool65536[POOL65536_LEN][8192];
uint64_t *freeBlocks[NUM_POOLS] = { 0, 0, 0, 0, 0, 0, 0 }; // alloc blocks from and return to head of list
uint64_t *const pools[NUM_POOLS] = { pool16[0], pool64[0], pool256[0], pool1024[0], pool4096[0], pool16384[0], pool65536[0] };
size_t const blockSizes[NUM_POOLS] = { sizeof pool16[0], sizeof pool64[0], sizeof pool256[0], sizeof pool1024[0], sizeof pool4096[0], sizeof pool16384[0], sizeof pool65536[0] };
size_t const poolSizes[NUM_POOLS] = { sizeof pool16, sizeof pool64, sizeof pool256, sizeof pool1024, sizeof pool4096, sizeof pool16384, sizeof pool65536 };
size_t const poolLens[NUM_POOLS] = { poolSizes[0] / blockSizes[0], poolSizes[1] / blockSizes[1], poolSizes[2] / blockSizes[2], poolSizes[3] / blockSizes[3], poolSizes[4] / blockSizes[4], poolSizes[5] / blockSizes[5], poolSizes[6] / blockSizes[6] };

#define NUM_HASH_ENTRIES 256
static ObjPtr hashTable[NUM_HASH_ENTRIES];

static void returnPtrEntry(PtrEntry *);
static PtrEntry *allocPtrEntry(void);
static void noLongerCopied(PtrEntry *);
static void noLongerACopy(PtrEntry *);

// these bracket calls to alloc, owned, free, copy, modify and gc
static void lock(void);
static void unlock(void);

// string support
static uint8_t hashString(char const *, ArrayItemCount);
static ObjPtr storeString(ObjPtr);

void osInit(void) {
    // create pools - for now we just use malloc

    // create ptr entry free list
    for (int i = 0; i < NUM_PTR_ENTRIES; i++) {
        returnPtrEntry(&ptrEntries[i]);
    }

    for (int i = 0; i < NUM_POOLS; i++) {
        freeBlocks[i] = pools[i];
        uint64_t **next = (void *)freeBlocks[i];
        for (size_t o = 0; o < poolLens[i] - 1; o++) {
            *next = *next + blockSizes[i] / sizeof (uint64_t);
            next = (void *)*next;
        }
        *next = 0; // end of free list
    }

    storeString(OBJ_PTR_THIS);
}

void osExtend(void) {}

ObjPtr osAlloc(ObjType t, ObjSize sz) {
    PtrEntry *newPe = allocPtrEntry();
    int pool = 0;
    while (poolSizes[pool] < sz || freeBlocks[pool] == 0) {
        pool++;
    }
    lock();
    uint64_t **block = (void *)freeBlocks[pool];
    freeBlocks[pool] = *block;
    unlock();
    *newPe = (PtrEntry){ .obj = block, .sz = sz, .copyOf = 0, .objType = t, .pool = pool, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = false };

    return newPe - ptrEntries;
}

void osNoGc(ObjPtr p) {
    assert(p < NUM_PTR_ENTRIES);
    assert(!ptrEntries[p].locked);
    ptrEntries[p].locked = true;
}

void osGcOk(ObjPtr p) {
    assert(p < NUM_PTR_ENTRIES);
    assert(ptrEntries[p].locked);
    ptrEntries[p].locked = false;
}

void osRealloc(ObjPtr p, ObjSize sz) {
    assert(p < NUM_PTR_ENTRIES);
    PtrEntry *pe = &ptrEntries[p];
    ObjSize capacty = pe->pool < OS_POOL_NONE ? 0 : poolLens[pe->pool];
    if (sz < capacty) {
        pe->sz = sz;
    } else {
        int pool = 0; // duplicate code
        while (poolSizes[pool] < sz || freeBlocks[pool] == 0) {
            pool++;
        }
        lock();
        uint64_t **block = (void *)freeBlocks[pool];
        freeBlocks[pool] = *block;
        unlock();

        memcpy(block, pe->obj, pe->sz);

        lock(); // duplicate code
        uint64_t *nextFree = freeBlocks[pe->pool];
        freeBlocks[pe->pool] = (void *)pe->obj;
        *(uint64_t **)pe->obj = nextFree;
        unlock();

        pe->obj = block;
        pe->sz =sz;
    }
}

void osFree(ObjPtr p) {
    assert(p < NUM_PTR_ENTRIES);
    PtrEntry *pe = &ptrEntries[p];

    assert(!pe->locked);
    assert(!pe->romEntry);

    if (pe->copy == OS_COPY_SOURCE) {
        noLongerCopied(pe);
    }
    else if (pe->copy == OS_COPY_COPY) {
        noLongerACopy(pe);
    }
    else switch (pe->pool) {
    case OS_POOL_NONE:
        break;
    default: {
        lock();
        uint64_t *nextFree = freeBlocks[pe->pool];
        freeBlocks[pe->pool] = (void *)pe->obj;
        *(uint64_t **)pe->obj = nextFree;
        unlock();
        break;
    }
    }

    lock();
    returnPtrEntry(pe);
    unlock();
}

ObjPtr osCopy(ObjPtr p) {
    PtrEntry *from = &ptrEntries[p];
    ObjPtr newP;
    if (from->copy == OS_COPY_UNIQUE || from->pool == OS_POOL_NONE ) { // for now only allow one copy, however PtrEntry::copy, could be a list and allow multiple copies
        PtrEntry *newPe = allocPtrEntry();
        *newPe = *from;
        if (from->pool != OS_POOL_NONE) {
            newPe->copyOf = p;
            newPe->copy = OS_COPY_COPY;
            from->copy = OS_COPY_SOURCE;
            from->copyOf = newPe - ptrEntries;
        }
        newP = newPe - ptrEntries;
    } else {
        newP = osAlloc(from->objType, from->sz);
        PtrEntry *newPe = &ptrEntries[newP];
        memcpy(&newPe->obj, &from->obj, from->sz);
    }
    return newP;
}

ObjPtr osStoreRomObject(void *obj, ObjType t, ObjSize sz) {
    PtrEntry *newPe = allocPtrEntry();
    *newPe = (PtrEntry){ .obj = obj, .sz = sz, .copyOf = 0, .objType = t, .pool = OS_POOL_NONE, .copy = OS_COPY_UNIQUE, .locked = false, .romEntry = false };
    return newPe - ptrEntries;
}

void osGc(void) {}

void const *osDeref(ObjPtr p) {
    assert(p < NUM_PTR_ENTRIES);
    PtrEntry *pe = &ptrEntries[p];
    assert(pe->pool != OS_POOL_NONE || pe->sz > 0); // only deref ram objs or rom objs, i.e. not zero length objs (type tags/nil)
   return pe->obj;
}

void *osDerefAndModify(ObjPtr p) {
    assert(p < NUM_PTR_ENTRIES);
    PtrEntry *pe = &ptrEntries[p];
    assert(!pe->romEntry);
    assert(pe->pool != OS_POOL_NONE); // only modify ram objs

    if (pe->copy != OS_COPY_UNIQUE) { // make a lazy copy
        ObjPtr copyOrCopiedPtr = pe->copyOf;
        PtrEntry *copyOrCopied = &ptrEntries[copyOrCopiedPtr];
        assert(copyOrCopied->copyOf == p);

        copyOrCopied->copy = pe->copy = OS_COPY_UNIQUE;
        copyOrCopied->copyOf = pe->copyOf = 0;

        int pool = 0; // duplicate code
        while (poolSizes[pool] < pe->sz || freeBlocks[pool] == 0) {
            pool++;
        }
        lock();
        uint64_t **block = (void *)freeBlocks[pool];
        freeBlocks[pool] = *block;
        unlock();

        memcpy(block, pe->obj, pe->sz);
        pe->obj = block;

        unlock();

    }
    return pe->obj;
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
    ObjPtr newP = osAlloc(OBJ_STRING, sizeof (ObjString) + l);
    PtrEntry *newPe = &ptrEntries[newP];
    size_t capacity = poolSizes[newPe->pool] - sizeof (ObjString);
    ObjString *so = newPe->obj;
    so->sLength = strlen(s);
    assert(capacity >= so->sLength);
    if (capacity > so->sLength) {
        memcpy(so->chars, s, so->sLength + 1);
    } else {
        memcpy(so->chars, s, so->sLength);
    }

    return storeString(newP);
}

ObjPtr osStoreRomCString(char const *s) {
    ObjPtr newP = osAlloc(OBJ_STRING | OBJ_IN_ROM, sizeof (ObjRomString));
    PtrEntry *newPe = &ptrEntries[newP];
    ObjRomString *so = newPe->obj;
    so->cStr = s;

    return storeString(newP);
}

char const *osStringAsCString(ObjPtr p) {
    ObjType t = osType(p);
    if (t == (OBJ_STRING | OBJ_IN_ROM)) {
        ObjRomString const *s = osDeref(p);
        return s->cStr;
    }
    else {
        assert(t == OBJ_STRING);
        PtrEntry *pe = &ptrEntries[p];
        size_t capacity = poolSizes[pe->pool] - sizeof (ObjString);
        ObjString const *s = (ObjString *)pe->obj;

        if (s->sLength < capacity) {
            return s->chars;
        } else { // if (sLength == sCapacity) {
            return osStringAsCString(osStoreCString(s->chars, s->sLength + 1));
        }
    }
}

void deleteString(ObjPtr p) {
    osNoGc(p);
    void const *s = osDeref(p);
    char const *sa;
    ArrayItemCount sl;
    ObjType st = osType(p);
    if (st == (OBJ_STRING | OBJ_IN_ROM)) { // todo - should rom strings be retained?
        sa = ((ObjRomString const *)s)->cStr;
        sl = strlen(sa);
    } else {
        assert(st == OBJ_STRING);
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
        ObjType tt = osType(p);
        osNoGc(hashHead);
        if (tt == (OBJ_IN_ROM | OBJ_STRING)) {
            ta = ((ObjRomString const *)s)->cStr;
            tl = strlen(sa);
        } else {
            assert(tt == OBJ_STRING);
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

static void returnPtrEntry(PtrEntry *pe) {
    pe->obj = (void *)1;
    pe->nextFreeEntry = freePtrEntries;
    freePtrEntries = pe;
}

static PtrEntry *allocPtrEntry(void) {
    assert(freePtrEntries != 0);

    lock();
    PtrEntry *r = freePtrEntries;
    freePtrEntries = r->nextFreeEntry;
    r->nextFreeEntry = (PtrEntry *)1; // debug only
    unlock();

    return r;
}

static void noLongerCopied(PtrEntry *pe) {
}

static void noLongerACopy(PtrEntry *pe) {
}

static void lock(void) {}

static void unlock(void) {}

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
    void const *s = osDeref(p);
    char const *sa;
    ArrayItemCount sl;
    ObjType st = osType(p);
    if (st == (OBJ_STRING | OBJ_IN_ROM)) {
        sa = ((ObjRomString const *)s)->cStr;
        sl = strlen(sa);
    } else {
        assert(st == OBJ_STRING);
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
        ObjType tt = osType(hashHead);
        osNoGc(hashHead);
        if (tt == (OBJ_IN_ROM | OBJ_STRING)) {
            ta = ((ObjRomString const *)s)->cStr;
            tl = strlen(sa);
        } else {
            assert(tt == OBJ_STRING);
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
