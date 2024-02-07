//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "ObjectHeader.h"
#include "Ray.h"

constexpr static CONSTANT size_t kNbObjectsMax = 64;
using TPrimitiveOffset = uint16_t;

class ObjectHeadersArray final
{
public:
    
    ObjectHeadersArray(CONSTANT uint8_t* buffer)
    : _buffer(buffer)
    {}
    
    size_t nbObjects() const { return _nbObjects; }
    
    CONSTANT ObjectHeader* header(size_t objectIndex) const
    {
        const size_t offset = _headerOffset[objectIndex];
        CONSTANT uint8_t* headerPtr = _buffer + offset;
        return reinterpret_cast<CONSTANT ObjectHeader*>(headerPtr);
    }
    
    void add(CONSTANT ObjectHeader* header)
    {
        const size_t offset = reinterpret_cast<CONSTANT uint8_t*>(header) - _buffer;
        _headerOffset[_nbObjects++] = TOffset(offset);
    }
    
private:
    CONSTANT uint8_t* _buffer;
    size_t _nbObjects = 0;
    
    using TOffset = uint16_t;
    TPrimitiveOffset _headerOffset[kNbObjectsMax];
};

