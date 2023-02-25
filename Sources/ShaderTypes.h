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
#else
    using EnumBackingType = int32_t;
    #define VB_ATTRIBUTE(a)
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
    float farZ;
    matrix_float4x4 modelViewMatrix;
};

#endif /* ShaderTypes_h */

