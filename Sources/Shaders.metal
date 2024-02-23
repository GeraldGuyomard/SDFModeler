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

vertex VertexShaderOut vertexShaderSDF(Vertex in [[stage_in]])
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

fragment FragmentShaderOut fragmentShaderSDF(VertexShaderOut in [[stage_in]],
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

struct FragmentShaderOut_ColorOnly
{
    float4 color [[color(0)]];
};

fragment FragmentShaderOut_ColorOnly fragmentShaderMatting(VertexShaderOut in [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(BufferIndexUniforms) ]],
                               constant SerializedWorldObject& serializedWorld [[ buffer(BufferIndexSerializedWorld) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    const auto res = render<MattingShader, NullEnvironment<MattingShader>, false>(in.viewportNDC, uniforms, serializedWorld, materials);
    
    FragmentShaderOut_ColorOnly out;
    out.color = res.color;
    
    return out;
}

struct VertexShader_BlurOut
{
    float4 position [[position]];
    float2 textCoords;
};

vertex VertexShader_BlurOut vertexShaderBlur(VertexShader_BlurIn in [[stage_in]])
{
    VertexShader_BlurOut out;

    out.position = in.position;
    out.textCoords = in.textCoords;

    return out;
}


struct BlurOut
{
    float4 color [[color(0)]];
};

fragment BlurOut fragmentShaderBlur(VertexShader_BlurOut in [[stage_in]],
                                    texture2d<float> inTexture [[ texture(TextureIndexInput) ]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                   mag_filter::linear,
                                   min_filter::linear);
    
    BlurOut out;
    
    constexpr float d = 1.f / 500.f;
    
    const auto c = inTexture.sample(colorSampler, in.textCoords);
    
    const auto c0 = inTexture.sample(colorSampler, in.textCoords - float2 { -d, -d } );
    const auto c1 = inTexture.sample(colorSampler, in.textCoords + float2 { +d, -d } );
    const auto c2 = inTexture.sample(colorSampler, in.textCoords + float2 { +d, +d } );
    const auto c3 = inTexture.sample(colorSampler, in.textCoords + float2 { -d, +d } );
    
    const auto color = (c + c0 + c1 + c2 + c3) / 5.f;
    out.color = color - c;
    
    return out;
}
