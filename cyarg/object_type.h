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

typedef struct ValueCell {
    ObjPtr value;
    ObjPtr type;
} ValueCell;

typedef struct ValueCollectionEntry {
    ObjPtr key;
    ObjPtr value;
} ValueCollectionEntry;

typedef struct TypedCollectionEntry {
    ObjPtr key;
    ObjPtr value;
    ObjPtr type;
} TypedCollectionEntry;

#endif
