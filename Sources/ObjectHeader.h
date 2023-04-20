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

struct ObjectHeader;
using ConstantObjectHeader = CONSTANT ObjectHeader;

struct ObjectHeader final
{
    uint32_t    byteSize;
    uint16_t    materialId;
    uint8_t     selected = false;
    uint8_t     operation = uint16_t(SDFOperation::addition);
    
    uint32_t    objectId;
    uint32_t    objectCode;
    
    uint8_t     firstByte;
    
    SDFOperation sdfOperation() CONSTANT
    {
        return (SDFOperation) operation;
    }
    
    static CONSTANT ObjectHeader* next(CONSTANT ObjectHeader* header)
    {
        CONSTANT uint8_t* ptr = reinterpret_cast<CONSTANT uint8_t*>(header);
        return reinterpret_cast<CONSTANT ObjectHeader*>(ptr + header->byteSize);
    }
    
    static DEVICE ObjectHeader* next(DEVICE ObjectHeader* header)
    {
        DEVICE uint8_t* ptr = reinterpret_cast<DEVICE uint8_t*>(header);
        return reinterpret_cast<DEVICE ObjectHeader*>(ptr + header->byteSize);
    }
};

template <typename TPrimitive>
INLINE CONSTANT TPrimitive* typedPrimitive(CONSTANT ObjectHeader* header)
{
    CONSTANT uint8_t* firstBytePtr = &(header->firstByte);
    return reinterpret_cast<CONSTANT TPrimitive*>(firstBytePtr);
}


