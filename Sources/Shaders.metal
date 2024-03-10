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
    
#if 1
    out.color = res.color;
    out.depth = res.adjustedDepth(uniforms.inverseZ());
    
#else
    const float2 uv = (in.viewportNDC + float2 { 1.f, 1.f }) * 0.5f;
    out.color = float4 { uv.x, uv.y, 0, 1 };
#endif
    
    return out;
}

struct FragmentShaderOut_Matting
{
    float depth [[depth(any)]];
};

fragment FragmentShaderOut_Matting fragmentShaderMatting(VertexShaderOut in [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(BufferIndexUniforms) ]],
                               constant SerializedWorldObject& serializedWorld [[ buffer(BufferIndexSerializedWorld) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    const auto res = render<MattingShader, NullEnvironment<MattingShader>, true /*write to depth*/>(in.viewportNDC, uniforms, serializedWorld, materials);
    
    FragmentShaderOut_Matting out;
    out.depth = res.adjustedDepth(uniforms.inverseZ());
    
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
    float depth [[depth(any)]];
};

fragment FragmentShader_SelectionOutlineOut fragmentShaderOutline(VertexShader_SelectionOutlineOut in [[stage_in]],
                                    constant OutlineUniforms& uniforms [[ buffer(BufferIndexOutlineUniforms) ]],
                                    texture2d<float> mainDepthTexture [[ texture(MainDepthTextureIndex) ]],
                                    texture2d<float> mattingDepthTexture [[ texture(MattingDepthIndexInput) ]]
                                    )
{
    constexpr sampler depthSampler(mip_filter::linear,
                                   mag_filter::linear,
                                   min_filter::linear);
    
    const auto delta = uniforms.samplingDelta;
    
    const float2 samplingRange = 2.f * delta;
    size_t n = 0;
    float mattingDepth = 0.f;
    
    for (float x = -delta.x; x <= delta.x; x += samplingRange.x)
    {
        for (float y = -delta.y; y <= delta.y; y += samplingRange.y)
        {
            const auto d = mattingDepthTexture.sample(depthSampler, in.textCoords + float2 { x, y }).r;
            mattingDepth = max(mattingDepth, d);
            
            if (d != 0.f)
            {
                ++n;
            }
        }
    }
    
    FragmentShader_SelectionOutlineOut out;
    
    const float originalDepth = mainDepthTexture.sample(depthSampler, in.textCoords).r;
    
    if ((n > 0) && (n < 4))
    {
        out.depth = mattingDepth;
        
        out.color = uniforms.color;
        out.color.a = 1.f;
        
        if (originalDepth >= mattingDepth)
        {
            out.color.a *= 0.25f;
        }
    }
    else
    {
        discard_fragment();
        
        // keep blending
        out.color = float4 { 0.f, 0.f, 0.f, 0.f };
        out.depth = originalDepth;
    }
    
    return out;
}
