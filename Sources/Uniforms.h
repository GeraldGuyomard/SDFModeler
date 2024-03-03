//
//  Uniforms.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"

static CONSTANT constexpr size_t kLeftCameraIndex = 0;

struct Uniforms final
{
    float4x4 worldTransformToNdc;
    float4x4 ndcToWorldTransform;
    float nearZInNDC = 0.f;
    float farZInNDC = 0.5f;
    float rayLength = 100.f;
    
    float3 lightDirection;
    
    float2 viewportSize;
};

INLINE float3 viewToWorld(float2 ndc, float z, CONSTANT Uniforms& uniforms)
{
    auto p = uniforms.ndcToWorldTransform * float4 { ndc.x, ndc.y, z, 1 };
    if (p.w != 0.f) {
        return p.xyz / p.w;
    } else {
        return float3 { p.x, p.y, p.z };
    }
}

struct OutlineUniforms final
{
    float2 samplingDelta;
    float4 color;
};
