//
//  ShaderTypes.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

//
//  Header containing types and enum constants shared between Metal shaders and Swift/ObjC source
//
#ifndef ShaderTypes_h
#define ShaderTypes_h

#ifdef __METAL_VERSION__

    using EnumBackingType = metal::int32_t;
    #define VB_ATTRIBUTE(a) [[attribute(a)]]
    #define CONSTANT constant

#else

    using EnumBackingType = int32_t;
    #define VB_ATTRIBUTE(a)
    #define CONSTANT const

    simd_float4x4 operator*(simd_float4x4 lhs, simd_float4x4 rhs)
    {
        return simd_mul(lhs, rhs);
    }

    simd_float4 operator*(simd_float4x4 lhs, simd_float4 rhs)
    {
        return simd_mul(lhs, rhs);
    }

#endif

#include <simd/simd.h>

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
    matrix_float4x4 invProjectionMatrix;
    matrix_float4x4 cameraMatrix;
};

simd_float3 viewToWorld(simd_float2 ndc, float z, CONSTANT Uniforms& uniforms)
{
    auto p = uniforms.cameraMatrix * uniforms.invProjectionMatrix * simd_float4 { ndc.x, ndc.y, z, 1 };
    return p.xyz / p.w;
}

#endif /* ShaderTypes_h */

