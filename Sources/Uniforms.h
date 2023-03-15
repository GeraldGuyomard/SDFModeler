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

enum class ObjectType : uint32_t
{
    sphere = 0,
    box = 1
};

struct DynamicScene final
{
    uint32_t size;
    uint8_t buffer[2048];
};


INLINE float3 viewToWorld(float2 ndc, float z, CONSTANT Uniforms& uniforms)
{
    auto p = uniforms.ndcToWorldTransform * float4 { ndc.x, ndc.y, z, 1 };
    return p.xyz / p.w;
}
