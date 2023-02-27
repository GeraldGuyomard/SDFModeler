//
//  ShaderTypes.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

//
//  Header containing types and enum constants shared between Metal shaders and Swift/ObjC source
//
#pragma once

#include "CommonDefinitions.h"

enum BufferIndex : EnumBackingType
{
    BufferIndexMeshPositions    = 0,
    BufferIndexMeshViewportNDCs = 1,
    BufferIndexUniforms         = 2
};

enum VertexAttribute : EnumBackingType
{
    VertexAttributePosition  = 0,
    VertexAttributeViewportNDC  = 1,
};

struct Vertex final
{
    simd_float4 position VB_ATTRIBUTE(VertexAttributePosition);
    simd_float2 viewportNDC VB_ATTRIBUTE(VertexAttributeViewportNDC);
};

struct Uniforms final
{
    simd_float4x4 invProjectionMatrix;
    simd_float4x4 cameraMatrix;
    simd_float4x4 ndcToWorldTransform;
};

simd_float3 viewToWorld(simd_float2 ndc, float z, CONSTANT Uniforms& uniforms)
{
    auto p = uniforms.ndcToWorldTransform * simd_float4 { ndc.x, ndc.y, z, 1 };
    return p.xyz / p.w;
}


