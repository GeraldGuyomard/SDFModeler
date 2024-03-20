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
#include "CameraUniforms.h"

enum BufferIndex : EnumBackingType
{
    BufferIndexMeshPositions    = 0,
    BufferIndexMeshViewportNDCs = 1,
    
    BufferIndexViewDependentUniforms   = 2,
    BufferIndexMaterials                = 3,
    
    BufferIndexUVs = 1,
    
    BufferIndexOutlineUniforms = 0,
    BufferIndexRasterizationRateMapUniforms = 4,
    
    BufferIndexWorkingPlaneUniform = 0
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
    MainDepthTextureIndex = 0,
    MattingDepthIndexInput = 1
};

struct VertexShader_SelectionOutlineIn
{
    simd_float4 position VB_ATTRIBUTE(VertexAttributePosition);
    float2 textCoords VB_ATTRIBUTE(VertexAttributeTexcoord);
};

struct VertexShader_WorkingPlaneIn
{
    simd_float4 position VB_ATTRIBUTE(VertexAttributePosition);
    float2 textCoords VB_ATTRIBUTE(VertexAttributeTexcoord);
};

