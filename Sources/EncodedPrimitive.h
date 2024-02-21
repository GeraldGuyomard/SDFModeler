//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectType.h"
#include "Transformer/Transformer.h"

CONSTANT static constexpr uint64_t kObjectTypeShift = 2;

INLINE constexpr uint32_t computeObjectCode(ObjectType objectType, TransformerType transformerType)
{
    return (uint32_t(objectType) << kObjectTypeShift) | uint32_t(transformerType);
}

template <typename TObject, typename TTransformer>
INLINE constexpr uint64_t computeObjectCode()
{
    return computeObjectCode(TObject::objectType(), TTransformer::transformerType());
}

struct EncodedPrimitive final
{
    uint16_t    _unused;
    uint8_t     selected;
    uint8_t     operation;
    
    uint32_t    objectId;
    uint32_t    materialId;
    uint32_t    objectCode;
    
    uint8_t     firstByte;
    
    SDFOperation sdfOperation() CONSTANT
    {
        return (SDFOperation) operation;
    }
};

template <typename TPrimitive>
INLINE CONSTANT TPrimitive* typedPrimitive(CONSTANT EncodedPrimitive* primitive)
{
    CONSTANT uint8_t* firstBytePtr = &(primitive->firstByte);
    return reinterpret_cast<CONSTANT TPrimitive*>(firstBytePtr);
}


