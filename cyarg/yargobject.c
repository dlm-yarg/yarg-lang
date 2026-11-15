#include "yargobject.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "memory.h"
#include "yargstructtype.h"
#include "packed_containers.h"
#include "vm.h"

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    initFunction(function);
    return function;
}

void initFunction(ObjFunction* function) {
    // may not be called after alloc, so init all fields.
    function->arity = 0;
    function->upvalueCount = 0;
    function->fName = NULL;
    initChunk(&function->chunk);
}

ObjString* functionToString(const ObjFunction* function) {
    if (function->fName == NULL) {
        return copyString("<script>", 8);
    }
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "<fn %s>", function->fName->chars);
    return copyString(buffer, (int)strlen(buffer));
}

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
#if defined CYARG_ORDERED_STRING_TREE
    keyValueStoreInit(&klass->methods);
#else
    initTable(&klass->methods);
#endif
    return klass;
}

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
#if defined CYARG_ORDERED_STRING_TREE
    keyValueStoreInit(&instance->fields);
#else
    initTable(&instance->fields);
#endif
    return instance;
}

ObjInt* allocateIntObject(size_t numDigits) {
    numDigits += numDigits % 2; // numDigits is always even
    assert(numDigits <= 254 && numDigits >= 2);
    ObjInt *i = (ObjInt *) allocateObject(sizeof (ObjInt) + numDigits * sizeof (uint16_t), OBJ_INT);
    i->bigInt.m_ = numDigits;
    i->bigInt.obj_ = true;
    return i;
}

Value defaultIntValue() {
    Value i = { .type = &yargTypes.integer };
    (void) int_init_concrete2(&i.as.bigSmallInt.i_);
    return i;
}

void noLongerLiteralInt(Value *value) {
    if (IS_INT(*value)) {
        AS_BIG_SMALL_INT(*value).literal_ = false;
        AS_INT(*value)->literal_ = false; // in the small case this will repeat the assignment above, in the large case there is literal_ in the value and the object
    }
}

ObjMap* newMap(ObjConcreteYargTypeMap* type) {
    ObjMap* map = ALLOCATE_OBJ(ObjMap, OBJ_MAP);
    map->type = type;
#if defined CYARG_ORDERED_STRING_TREE
    keyValueStoreInit(&map->entries);
#else
    initTable(&map->entries);
#endif
    return map;
}

ObjString* mapToString(const ObjMap* map) {
    ObjString* typeStr = valueToString(YARGTYPE_VAL(map->type));
    char buffer[64];
#if defined CYARG_ORDERED_STRING_TREE
    snprintf(buffer, sizeof(buffer), "<map (%d) %s >", keyValueStoreCount(&map->entries), typeStr->chars);
#else
    snprintf(buffer, sizeof(buffer), "<map (%d) %s >", map->entries.count, typeStr->chars);
#endif
    return copyString(buffer, (int)strlen(buffer));
}

bool isAddressValue(Value val) {
    if (IS_INT(val)) {
        return AS_BIG_SMALL_INT(val).literal_;
    } else if (IS_ADDRESS(val)) {
        return true;
    } else {
        return false;
    }
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
#if defined CYARG_ORDERED_STRING_TREE
    string = stringStoreStore(&vm.stringStore, string);
#else
    string->hash = hash;
    tempRootPush(STRING_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    tempRootPop();
#endif
    return string;
}

#if !defined CYARG_ORDERED_STRING_TREE
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash += 16777619;
    }
    return hash;
}
#endif // !defined CYARG_ORDERED_STRING_TREE

ObjString* takeString(char* chars, int length) {
#if defined CYARG_ORDERED_STRING_TREE
    ObjString *s = stringStoreGet(&vm.stringStore, chars);
    if (s == NULL) {
        s = allocateString(chars, length, 0u);
    } else {
        vm_free(chars);
    }
    return s;
#else
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        vm_free(chars);
        return interned;
    }

    return allocateString(chars, length, hash);
#endif
}

ObjString* copyString(const char* chars, int length) {
#if defined CYARG_ORDERED_STRING_TREE
    ObjString *s = stringStoreGet(&vm.stringStore, chars);
    if (s == NULL) {
        char *heapChars = ALLOCATE(char, length + 1);
        memcpy(heapChars, chars, length);
        heapChars[length] = '\0';
        s = allocateString(heapChars, length, 0u);
    }
    return s;
#else
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
#endif
}

ObjString* copyStringWithEscapes(const char* chars, int length)
{
    char* heapChars = ALLOCATE(char, length + 1);

    char const *in = chars;
    char *out = heapChars;
    int lengthOut = 0;
    for (int i = 0; i < length && *in != '\0'; i++)
    {
        if (*in == '\\')
        {
            in++;
            i++;
        }
        *out++ = *in++;
        lengthOut++;
    }
#if defined CYARG_ORDERED_STRING_TREE
    *out = '\0';

    return takeString(heapChars, lengthOut);
#else
    uint32_t hash = hashString(heapChars, lengthOut);
    ObjString* interned = tableFindString(&vm.strings, heapChars, lengthOut, hash);
    if (interned != NULL)
    {
        vm_free(heapChars);
        return interned;
    }

    *out = '\0';
    return allocateString(heapChars, lengthOut, hash);
#endif
}

Value cstring_val(const char *cstring) {
    return STRING_VAL(copyString(cstring, (int) strlen(cstring)));
}
