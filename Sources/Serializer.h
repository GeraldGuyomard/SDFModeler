//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "EncodedPrimitive.h"
#include "Material/Material.h"

#include <cstring>
#include <assert.h>

template <typename TObject>
static size_t copy(ObjectID id,
                MaterialID materialId,
                 ObjectType objectType,
                 TransformerType transformerType,
                 EncodedPrimitive* encodedPrimitive,
                 const TObject& object,
                 SDFOperation operation,
                 bool selected)
{
    assert(objectType != ObjectType::invalid);
    
    const size_t size = sizeof(TObject);
    
    encodedPrimitive->objectId = id;
    encodedPrimitive->materialId = materialId;
    encodedPrimitive->objectCode = computeObjectCode(objectType, transformerType);
    encodedPrimitive->operation = uint16_t(operation);
    encodedPrimitive->selected = selected;
    
    uint8_t* dst = &(encodedPrimitive->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
    
    return alignedSize(size);
}

template <typename TObject>
static size_t copy(EncodedPrimitive* primitive,
                 const TObject& object,
                 ObjectID id,
                MaterialID materialId,
                 ObjectType objectType,
                 TransformerType transformerType,
                 SDFOperation operation,
                 bool selected)
{
    return copy<TObject>(id, materialId, objectType, transformerType, primitive, object, operation, selected);
}


template <typename TPrimitive>
INLINE size_t encodePrimitive(EncodedPrimitive* encodedPrimitive,
                              const TPrimitive& primitive,
                              ObjectID id,
                              MaterialID materialId,
                              ObjectType objectType,
                              TransformerType transformerType,
                              SDFOperation operation,
                              bool selected)
{
    return copy(encodedPrimitive, primitive, id, materialId, objectType, transformerType, operation, selected);
}
