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

struct VertexShader_SelectionOutlineOut
{
    float4 position [[position]];
    float2 textCoords;
};

vertex VertexShader_SelectionOutlineOut vertexShaderOutline(VertexShader_SelectionOutlineIn in [[stage_in]])
{
    VertexShader_SelectionOutlineOut out;

    out.position = in.position;
    out.textCoords = in.textCoords;

    return out;
}

struct FragmentShader_SelectionOutlineOut
{
    float4 color [[color(0)]];
};

fragment FragmentShader_SelectionOutlineOut fragmentShaderOutline(VertexShader_SelectionOutlineOut in [[stage_in]],
                                    texture2d<float> inTexture [[ texture(TextureIndexInput) ]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                   mag_filter::linear,
                                   min_filter::linear);
    
    FragmentShader_SelectionOutlineOut out;
    
    constexpr float d = 1.f / 500.f;
    
    const float c = inTexture.sample(colorSampler, in.textCoords).r;
    if (c != 0.f)
    {
        float color = 0.f;
        for (float x = -d; x <= d; x += d)
        {
            for (float y = -d; y <= d; y += d)
            {
                color += inTexture.sample(colorSampler, in.textCoords + float2 { x, y } ).r;
            }
        }
        
        color /= 9.f;
        
        if (color == 1.f)
        {
            discard_fragment();
        }
        
        
        color = clamp(color, 0.f, 1.f);
        
        const float outlineLevel = clamp(c - color, 0.f, 1.f);
        //const float3 outlineColor { 0.5, 0.5, 1 };
        const float3 outlineColor { 252. / 255., 202. / 255., 0. };
        
        out.color = { outlineColor.r, outlineColor.g, outlineColor.b, outlineLevel };
        
        return out;
    }

    discard_fragment();
    return {};
}
