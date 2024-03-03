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
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    
    float4x4 invProjectionMatrix;
    float4x4 cameraMatrix;
    
    float nearZInNDC = 0.f;
    float farZInNDC = 0.5f;
    float rayLength = 100.f;
    
    float3 lightDirection;
    
    float2 viewportSize;
    
    float4x4 worldTransformToNdc() CONSTANT
    {
        return viewMatrix * projectionMatrix;
    }
};

INLINE float3 viewToWorld(float2 ndc, float z, CONSTANT float4x4& cameraMatrix, CONSTANT float4x4& invProjMatrix)
{
    float4 ptInCameraSpace = invProjMatrix * float4 { ndc.x, ndc.y, z, 1 };
    if (ptInCameraSpace.w != 0.f)
    {
        ptInCameraSpace /= ptInCameraSpace.w;
    }
    
    ptInCameraSpace.w = 1.f;
    
    auto ptInWorldSpace = cameraMatrix * ptInCameraSpace;
    ASSERT(ptInWorldSpace.w == 1.f);
    
    return float3 { ptInWorldSpace.x, ptInWorldSpace.y, ptInWorldSpace.z };
}

struct OutlineUniforms final
{
    float2 samplingDelta;
    float4 color;
};
