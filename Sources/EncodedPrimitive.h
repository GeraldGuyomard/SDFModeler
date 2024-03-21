//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectType.h"
#include "Material/Material.h"
#include "Transformer/Transformer.h"

CONSTANT static constexpr uint64_t kObjectTypeShift = 2;

using TObjectCode = uint32_t;
INLINE constexpr TObjectCode computeObjectCode(ObjectType objectType, TransformerType transformerType)
{
    return (TObjectCode(objectType) << kObjectTypeShift) | TObjectCode(transformerType);
}

template <typename TObject, typename TTransformer>
INLINE constexpr TObjectCode computeObjectCode()
{
    return computeObjectCode(TObject::objectType(), TTransformer::transformerType());
}

struct EncodedPrimitive final
{
    uint16_t    operation;
    uint16_t    _unused; // for flags?
    float       blendingFactor;
    
    ObjectID    objectId; // 2
    MaterialID  materialId; // 2
    TObjectCode objectCode; // 2
    
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


