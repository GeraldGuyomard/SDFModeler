//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"
#include "Composition.h"

#include <cstring>
#include <assert.h>

template <typename TObject>
static void copy(ObjectType objectType, ObjectHeader* header, const TObject& object)
{
    const size_t size = sizeof(TObject);
    
    header->byteSize = alignedSize(size);
    header->objectType = objectType;
    assert(header->objectType != ObjectType::invalid);
    
    uint8_t* dst = &(header->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
}

template <typename TObject>
static void copy(ObjectHeader* header, const TObject& object, ObjectType objectType)
{
    copy<TObject>(objectType, header, object);
}


template <typename TPrimitive>
INLINE void serializeObject(uint8_t*& p, const TPrimitive& primitive, ObjectType objectType)
{
    ObjectHeader* h = (ObjectHeader*) p;
    copy(h, primitive, objectType);
    
    p += h->byteSize;
}
