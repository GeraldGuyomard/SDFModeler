//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#include "SerializationContext.h"

SerializationContext::SerializationContext(SerializedWorldObject& serializedWorld, const float4x4& viewProjectionMatrix)
: _serializedWorld(serializedWorld), _viewProjectionMatrix(viewProjectionMatrix)
{
    _serializedWorld.objectCount = 0;
    _availableObjectHeader = reinterpret_cast<ObjectHeader*>(&_serializedWorld.buffer[0]);
}


void
SerializationContext::serializeObjectHeader(const SerializationHeaderCallback& cb)
{
    const size_t size = cb(_availableObjectHeader);
    
    _availableObjectHeader->byteSize = uint32_t(size);
    _availableObjectHeader = ObjectHeader::next(_availableObjectHeader);
    
    ++_serializedWorld.objectCount;
}
