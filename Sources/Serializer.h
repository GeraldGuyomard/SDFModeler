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
static void copy(ObjectID id, ObjectType objectType, TransformerType transformerType, ObjectHeader* header, const TObject& object)
{
    assert(objectType != ObjectType::invalid);
    
    const size_t size = sizeof(TObject);
    
    header->byteSize = alignedSize(size);
    header->objectId = id;
    header->objectCode = computeObjectCode(objectType, transformerType);
    
    uint8_t* dst = &(header->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
}

template <typename TObject>
static void copy(ObjectHeader* header, const TObject& object, ObjectID id, ObjectType objectType, TransformerType transformerType)
{
    copy<TObject>(id, objectType, transformerType, header, object);
}


template <typename TPrimitive>
INLINE size_t serializeObject(uint8_t* p, const TPrimitive& primitive, ObjectID id, ObjectType objectType, TransformerType transformerType)
{
    ObjectHeader* h = (ObjectHeader*) p;
    copy(h, primitive, id, objectType, transformerType);
    
    return h->byteSize;
}
