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
static void copy(ObjectID id,
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
    
    header->byteSize = uint32_t(alignedSize(size));
    header->objectId = id;
    header->materialId = materialId;
    header->objectCode = computeObjectCode(objectType, transformerType);
    header->operation = uint16_t(operation);
    header->selected = selected;
    
    uint8_t* dst = &(header->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
}

template <typename TObject>
static void copy(ObjectHeader* header,
                 const TObject& object,
                 ObjectID id,
                 MaterialID materialId,
                 ObjectType objectType,
                 TransformerType transformerType,
                 SDFOperation operation,
                 bool selected)
{
    copy<TObject>(id, materialId, objectType, transformerType, header, object, operation, selected);
}


template <typename TPrimitive>
INLINE size_t serializeObject(ObjectHeader* header,
                              const TPrimitive& primitive,
                              ObjectID id,
                              MaterialID materialId,
                              ObjectType objectType,
                              TransformerType transformerType,
                              SDFOperation operation,
                              bool selected)
{
    copy(header, primitive, id, materialId, objectType, transformerType, operation, selected);
    
    return header->byteSize;
}
