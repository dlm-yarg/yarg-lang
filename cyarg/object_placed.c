//
//  object_placed.c
//  gh.yarg-lang
//
//  Created by dlm on 18/08/2026.
//

#include "object_placed.h"

#include "object_store.h"
#include "yargtype.h"

static ObjSize ytItemSize(ObjPtr t, ObjSize *alignment) {
    ObjSize r;
    ObjSize biggestItemAlignment = 0;

    switch (t) {
    case OBJ_PTR_I8_TYPE: case OBJ_PTR_UI8_TYPE: biggestItemAlignment = r = sizeof (int8_t); break;
    case OBJ_PTR_I16_TYPE: case OBJ_PTR_UI16_TYPE: biggestItemAlignment = r = sizeof (int16_t); break;
    case OBJ_PTR_I32_TYPE: case OBJ_PTR_UI32_TYPE: biggestItemAlignment = r = sizeof (int32_t); break;
    case OBJ_PTR_I64_TYPE: case OBJ_PTR_UI64_TYPE: biggestItemAlignment = r = sizeof (int64_t); break;
    default: {
        switch (osType(t)) {
        case OBJ_ARRAY: {
            // alignment is the alignment of the elements
            // size is the multiple of alignment suficient to hold all  elements
            ObjConcreteYargTypeArray const *ao = osDeref(t);
            r = ao->cardinality * ytItemSize(ao->element_type, &biggestItemAlignment);
            break;
        }
        case OBJ_STRUCT: {
            // alignment is the alignment of the most aligned member
            // size is the multiple of alignment suficient to hold all aligned members
            ObjConcreteYargTypeStruct const *yt = osDeref(t);
            ObjSize offset = 0;
            for (int i = 0; i < yt->elements.arrayLength; i++) {
                YargTypeStructElement const *v = (YargTypeStructElement const *)daAt(&yt->elements, i);
                ObjSize itemAlignment;
                ObjSize itemSize = ytItemSize(v->type, &itemAlignment);
                biggestItemAlignment = biggestItemAlignment < itemAlignment ? itemAlignment : biggestItemAlignment;
                ObjSize padding = offset % itemAlignment;
                if (padding > 0) padding = itemAlignment - padding;
                offset += padding;
                offset += itemSize;
            }
            ObjSize padding = offset % biggestItemAlignment;
            if (padding > 0) padding = biggestItemAlignment - padding;
            r = offset + padding;
            break;
        }

        default:
            biggestItemAlignment = 0; r = 0; break; // todo assert?
        }
    }
    }
    if (alignment != 0) *alignment = biggestItemAlignment;
    return r;
}
