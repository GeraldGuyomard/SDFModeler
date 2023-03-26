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

