//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"
#include "Material/Material.h"

#include <cstring>
#include <assert.h>

template <typename TObject>
static size_t copy(ObjectID id,
                   ObjectID partId,
                 MaterialID materialId,
                 ObjectType objectType,
                 TransformerType transformerType,
                 ObjectHeader* header,
                 const TObject& object,
                 SDFOperation operation,
                 bool selected)
{
    assert(objectType != ObjectType::invalid);
    
    const size_t size = sizeof(TObject);
    
    header->objectId = id;
    header->partId = partId;
    header->materialId = materialId;
    header->objectCode = computeObjectCode(objectType, transformerType);
    header->operation = uint16_t(operation);
    header->selected = selected;
    
    uint8_t* dst = &(header->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
    
    return alignedSize(size);
}

template <typename TObject>
static size_t copy(ObjectHeader* header,
                 const TObject& object,
                 ObjectID id,
                 ObjectID partId,
                 MaterialID materialId,
                 ObjectType objectType,
                 TransformerType transformerType,
                 SDFOperation operation,
                 bool selected)
{
    return copy<TObject>(id, partId, materialId, objectType, transformerType, header, object, operation, selected);
}


template <typename TPrimitive>
INLINE size_t serializeObject(ObjectHeader* header,
                              const TPrimitive& primitive,
                              ObjectID id,
                              ObjectID partId,
                              MaterialID materialId,
                              ObjectType objectType,
                              TransformerType transformerType,
                              SDFOperation operation,
                              bool selected)
{
    return copy(header, primitive, id, partId, materialId, objectType, transformerType, operation, selected);
}
