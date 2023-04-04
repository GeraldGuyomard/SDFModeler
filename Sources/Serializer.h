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
static void copy(ObjectID id,
                 ObjectType objectType,
                 TransformerType transformerType,
                 ObjectHeader* header,
                 const TObject& object,
                 bool selected)
{
    assert(objectType != ObjectType::invalid);
    
    const size_t size = sizeof(TObject);
    
    header->byteSize = uint32_t(alignedSize(size));
    header->objectId = id;
    header->objectCode = computeObjectCode(objectType, transformerType);
    header->selected = selected;
    
    uint8_t* dst = &(header->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
}

template <typename TObject>
static void copy(ObjectHeader* header,
                 const TObject& object,
                 ObjectID id, ObjectType objectType,
                 TransformerType transformerType,
                 bool selected)
{
    copy<TObject>(id, objectType, transformerType, header, object, selected);
}


template <typename TPrimitive>
INLINE size_t serializeObject(uint8_t* p,
                              const TPrimitive& primitive,
                              ObjectID id,
                              ObjectType objectType,
                              TransformerType transformerType,
                              bool selected)
{
    ObjectHeader* h = (ObjectHeader*) p;
    copy(h, primitive, id, objectType, transformerType, selected);
    
    return h->byteSize;
}
