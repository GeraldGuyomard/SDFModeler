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
#include "Uniforms.h"

enum BufferIndex : EnumBackingType
{
    BufferIndexMeshPositions    = 0,
    BufferIndexMeshViewportNDCs = 1,
    BufferIndexUVs = 1,
    
    BufferIndexUniforms         = 2,
    BufferIndexSerializedWorld  = 3,
    BufferIndexMaterials        = 4
};

enum VertexAttribute : EnumBackingType
{
    VertexAttributePosition  = 0,
    VertexAttributeViewportNDC  = 1,
    VertexAttributeTexcoord  = 2
};

struct Vertex final
{
    simd_float4 position VB_ATTRIBUTE(VertexAttributePosition);
    simd_float2 viewportNDC VB_ATTRIBUTE(VertexAttributeViewportNDC);
};

enum TextureIndex : EnumBackingType
{
    TextureIndexInput = 0
};

struct VertexShader_SelectionOutlineIn
{
    simd_float4 position VB_ATTRIBUTE(VertexAttributePosition);
    float2 textCoords VB_ATTRIBUTE(VertexAttributeTexcoord);
};

