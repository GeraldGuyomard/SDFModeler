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

INLINE constexpr uint64_t computeObjectCode(ObjectType objectType, TransformerType transformerType)
{
    return (uint64_t(objectType) << kObjectTypeShift) | uint64_t(transformerType);
}

template <typename TObject, typename TTransformer>
INLINE constexpr uint64_t computeObjectCode()
{
    return computeObjectCode(TObject::objectType(), TTransformer::transformerType());
}

struct ObjectHeader final
{
    size_t    byteSize;
    uint64_t  objectCode;
    
    uint8_t   firstByte;
    
    ObjectHeader(uint32_t byteSize, ObjectType objectType, TransformerType transformerType)
    : byteSize(byteSize), objectCode(computeObjectCode(objectType, transformerType))
    {}
    
    static CONSTANT ObjectHeader* next(CONSTANT ObjectHeader* header)
    {
        CONSTANT uint8_t* ptr = reinterpret_cast<CONSTANT uint8_t*>(header);
        return reinterpret_cast<CONSTANT ObjectHeader*>(ptr + header->byteSize);
    }
};

template <typename TPrimitive>
INLINE CONSTANT TPrimitive* typedPrimitive(CONSTANT ObjectHeader* header)
{
    CONSTANT uint8_t* firstBytePtr = &(header->firstByte);
    return reinterpret_cast<CONSTANT TPrimitive*>(firstBytePtr);
}

