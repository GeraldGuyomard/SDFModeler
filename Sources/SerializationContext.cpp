//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#include "SerializationContext.h"

SerializationContext::SerializationContext(
                                           const float4x4& viewProjectionMatrix,
                                           Tile& tile,
                                           ObjectHeader*& availableObjectHeader)
:
_viewProjectionMatrix(viewProjectionMatrix),
_tile(tile),
_availableObjectHeader(availableObjectHeader)
{
    _tile.objectCount = 0;
}


void
SerializationContext::serializeObjectHeader(const SerializationHeaderCallback& cb)
{
    const size_t size = cb(_availableObjectHeader);
    
    _availableObjectHeader->byteSize = uint32_t(size);
    _availableObjectHeader = ObjectHeader::next(_availableObjectHeader);
    
    ++_tile.objectCount;
}
