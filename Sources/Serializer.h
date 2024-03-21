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

struct EncodingPrimitiveParams final
{
    const ObjectID id;
    const MaterialID materialId;
    const ObjectType objectType;
    const TransformerType transformerType;
    const SDFOperation operation;
    const float blendingFactor;
};

template <typename TObject>
size_t encodePrimitive(const EncodingPrimitiveParams& params, EncodedPrimitive* encodedPrimitive, const TObject& object)
{
    assert(params.objectType != ObjectType::invalid);
    
    const size_t size = sizeof(TObject);
    
    encodedPrimitive->objectId = params.id;
    encodedPrimitive->materialId = params.materialId;
    encodedPrimitive->objectCode = computeObjectCode(params.objectType, params.transformerType);
    encodedPrimitive->operation = uint16_t(params.operation);
    encodedPrimitive->blendingFactor = params.blendingFactor;
    
    uint8_t* dst = &(encodedPrimitive->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&object);
    
    memcpy(dst, src, size);
    
    return alignedSize(size);
}

