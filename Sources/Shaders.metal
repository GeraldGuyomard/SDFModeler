//
//  Shaders.metal
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

// File for Metal kernel and shader functions


#include "RenderFunctions.h"

struct VertexShaderOut
{
    float4 position [[position]];
    float2 viewportNDC;
};

vertex VertexShaderOut vertexShader(Vertex in [[stage_in]])
{
    VertexShaderOut out;

    out.position = in.position;
    out.viewportNDC = in.viewportNDC;

    return out;
}

struct FragmentShaderOut
{
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

fragment FragmentShaderOut fragmentShader(VertexShaderOut in [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(BufferIndexUniforms) ]],
                               constant SerializedWorldObject& serializedWorld [[ buffer(BufferIndexSerializedWorld) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    const auto res = renderDefault(in.viewportNDC,
                         uniforms,
                         serializedWorld,
                         materials);
    
    FragmentShaderOut out;
    out.color = res.color;
    out.depth = res.depth;
    
    return out;
}
