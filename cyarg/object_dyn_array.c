#include "object_dyn_array.h"

#include <stddef.h>

#include "common.h"
#include "memory.h"
#include "object_gc_impl.h"

#include <memory.h>

// note that this call can be omitted for objects where
// the DynamicObjArray is a member, and Zero filled
void initDynamicObjArray(DynamicObjArray* array) {
    array->objects = NULL;
    array->objectCapacity = 0;
    array->objectCount = 0;
}

void freeDynamicObjArray(DynamicObjArray* array) {
    vm_free(array->objects);
    initDynamicObjArray(array);
}

void enlargeDynamicObjArray(DynamicObjArray* array, void* newObjArray, int newCapacity) {
    int oldCapacity = array->objectCapacity;
    if (newCapacity > oldCapacity) {
        array->objectCapacity = newCapacity;
        memcpy(newObjArray, array->objects, oldCapacity * sizeof (Obj *));
        void *oldObjArray = array->objects;
        array->objects = newObjArray;
        vm_free(oldObjArray);
    } else {
        vm_free(newObjArray);
    }
}

void appendToDynamicObjArray(DynamicObjArray* array, Obj* obj) {
    if (array->objectCapacity < array->objectCount + 1) {
        int newCapacity = GROW_CAPACITY(array->objectCapacity);
        void *newObjArray = vm_allocate(newCapacity * sizeof (Obj *));
        enlargeDynamicObjArray(array, newObjArray, newCapacity);
    }
    array->objects[array->objectCount] = obj;
    array->objectCount++;
}

void appendToDynamicObjArrayNoAlloc(DynamicObjArray* array, Obj* obj, void* newObjArray, int newCapacity) {
    if (newCapacity > array->objectCapacity) {
        enlargeDynamicObjArray(array, newObjArray, newCapacity);
    }
    array->objects[array->objectCount] = obj;
    array->objectCount++;
}
