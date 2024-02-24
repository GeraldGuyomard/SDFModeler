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
    float4x4 worldTransformToNdc;
    float4x4 ndcToWorldTransform;
    
    float3 lightDirection;
    
    float2 viewportSize;
};

INLINE float3 viewToWorld(float2 ndc, float z, CONSTANT Uniforms& uniforms)
{
    auto p = uniforms.ndcToWorldTransform * float4 { ndc.x, ndc.y, z, 1 };
    return p.xyz / p.w;
}

struct OutlineUniforms final
{
    float2 samplingDelta;
    float4 color;
};
