//
//  object_type.h
//  yarg-lang
//
//  Created by dlm on 13/05/2026.
//

#ifndef cyarg_object_type_h
#define cyarg_object_type_h

#include <stdint.h>

typedef uint16_t ObjPtr;
typedef uint16_t ObjSize;
typedef uint16_t ArrayItemCount;
typedef uint16_t ElementOffset;

// for maps/structs/classes
typedef struct KeyValue {
    ObjPtr key;
    ObjPtr value;
} KeyValue;

// for placed-structs
typedef struct KeyOffsetType {
    ObjPtr key;
    ElementOffset offset;
    ObjPtr type;
} KeyOffsetType;

#endif
