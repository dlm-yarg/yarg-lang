#ifndef cyarg_object_dyn_array_h
#define cyarg_object_dyn_array_h

typedef struct Obj Obj;

// Dynamic array of Object pointers. Takes ownership of the passed object before growing the array.
typedef struct DynamicObjArray {
    Obj** objects;
    int objectCapacity;
    int objectCount;
} DynamicObjArray;

void initDynamicObjArray(DynamicObjArray* array);
void freeDynamicObjArray(DynamicObjArray* array);

void appendToDynamicObjArray(DynamicObjArray* array, Obj* obj);
void enlargeDynamicObjArray(DynamicObjArray* array, void* newObjArray, int newCapacity);
void appendToDynamicObjArrayNoAlloc(DynamicObjArray* array, Obj* obj, void* newObjArray, int newCapacity);

#endif
