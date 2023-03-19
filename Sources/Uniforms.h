//
//  Uniforms.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"

struct Uniforms final
{
    float4x4 invProjectionMatrix;
    float4x4 cameraMatrix;
    float4x4 ndcToWorldTransform;
    
    float3 lightDirection;
};

enum class ObjectType : int64_t
{
    invalid = -1,
    
    sphere = 0,
    box = 1,
    roundedBox = 2,
    plane = 3
};

template <typename TSDFGeometry>
ObjectType getObjectType()
{
    return ObjectType::invalid;
}

struct ObjectHeader final
{
    size_t    byteSize;
    ObjectType  objectType;
    
    uint8_t     firstByte;
    
    ObjectHeader(uint32_t byteSize, ObjectType objectType)
    : byteSize(byteSize), objectType(objectType)
    {}
    
    template <typename TObject>
    static void copy(DEVICE ObjectHeader* header, CONSTANT TObject& object)
    {
        header->byteSize = sizeof(TObject);
        header->objectType = object.objectType();
        
        DEVICE TObject* dst = (DEVICE TObject*) &(header->firstByte);
        *dst = object;
    }
};

struct DynamicScene final
{
    uint64_t objectCount = 0;
    uint64_t padding = 0; // align so that buffer starts on 16 bytes alignment
    
    uint8_t buffer[2048];
};


INLINE float3 viewToWorld(float2 ndc, float z, CONSTANT Uniforms& uniforms)
{
    auto p = uniforms.ndcToWorldTransform * float4 { ndc.x, ndc.y, z, 1 };
    return p.xyz / p.w;
}
