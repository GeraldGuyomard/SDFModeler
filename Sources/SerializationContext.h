//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "SerializedWorldObject.h"
#include <functional>

class SerializationContext final
{
public:
    SerializationContext(SerializedWorldObject& serializedWorld, const float4x4& viewProjectionMatrix);
    
    const float4x4& viewProjectionMatrix() const { return _viewProjectionMatrix; }
    
    using SerializationHeaderCallback = std::function<size_t (ObjectHeader*)>;
    void serializeObjectHeader(const SerializationHeaderCallback&);
    
private:
    SerializedWorldObject& _serializedWorld;
    const float4x4 _viewProjectionMatrix;
    
    ObjectHeader* _availableObjectHeader;
};
