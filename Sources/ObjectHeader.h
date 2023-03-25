//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectType.h"

struct ObjectHeader final
{
    size_t    byteSize;
    ObjectType  objectType;
    
    uint8_t     firstByte;
    
    ObjectHeader(uint32_t byteSize, ObjectType objectType)
    : byteSize(byteSize), objectType(objectType)
    {}
};

template <typename TPrimitive>
INLINE CONSTANT TPrimitive* typedPrimitive(CONSTANT ObjectHeader* header)
{
    CONSTANT uint8_t* firstBytePtr = &(header->firstByte);
    return reinterpret_cast<CONSTANT TPrimitive*>(firstBytePtr);
}

template <typename TEvaluator, typename TPrimitive, typename TReturnValue>
INLINE TReturnValue evaluateTypedPrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    CONSTANT TPrimitive* prim = typedPrimitive<TPrimitive>(header);
    const TPrimitive p = *prim;
    return evaluator.evaluate(header, p);
}
