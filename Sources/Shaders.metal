//
//  Shaders.metal
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

// File for Metal kernel and shader functions


#include <TargetConditionals.h>
#include "RenderFunctions.h"
#include "ViewDependentUniforms.h"

struct VertexShaderOut
{
    float4 position [[position]];
    float2 viewportNDC;
    int cameraIndex;
};

vertex VertexShaderOut vertexShaderSDF(Vertex in [[stage_in]],
                                       ushort amp_id [[amplification_id]])
{
    VertexShaderOut out;

    out.position = in.position;
    out.viewportNDC = in.viewportNDC;
    out.cameraIndex = amp_id;

    return out;
}

struct FragmentShaderOut
{
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

fragment FragmentShaderOut fragmentShaderSDF(VertexShaderOut in [[stage_in]],
                               constant ViewDependentUniforms& viewDependentUniforms [[ buffer(BufferIndexViewDependentUniforms) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    CONSTANT auto& cameraUniforms = viewDependentUniforms.cameraUniforms[in.cameraIndex];
    CONSTANT auto& serializedWorld = viewDependentUniforms.serializedWorldObject[in.cameraIndex];
    const auto res = renderDefault(in.viewportNDC,
                         cameraUniforms,
                         serializedWorld,
                         materials);
    
    
    FragmentShaderOut out;
    
#if 1
    out.color = res.color;
    out.depth = res.adjustedDepth(cameraUniforms.inverseZ());
    
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
                                                         constant ViewDependentUniforms& viewDependentUniforms [[ buffer(BufferIndexViewDependentUniforms) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    CONSTANT auto& cameraUniforms = viewDependentUniforms.cameraUniforms[in.cameraIndex];
    CONSTANT auto& serializedWorld = viewDependentUniforms.serializedWorldObject[in.cameraIndex];
    
    const auto res = render<MattingShader, NullEnvironment<MattingShader>, true /*write to depth*/>(in.viewportNDC, cameraUniforms, serializedWorld, materials);
    
    FragmentShaderOut_Matting out;
    out.depth = res.adjustedDepth(cameraUniforms.inverseZ());
    
    return out;
}

struct VertexShader_SelectionOutlineOut
{
    float4 position [[position]];
    float2 textCoords;
    int cameraIndex;
};

vertex VertexShader_SelectionOutlineOut vertexShaderOutline(VertexShader_SelectionOutlineIn in [[stage_in]],
                                                            ushort amp_id [[amplification_id]])
{
    VertexShader_SelectionOutlineOut out;

    out.position = in.position;
    out.textCoords = in.textCoords;
    out.cameraIndex = amp_id;

    return out;
}

struct FragmentShader_SelectionOutlineOut
{
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

fragment FragmentShader_SelectionOutlineOut fragmentShaderOutline(VertexShader_SelectionOutlineOut in [[stage_in]],
                                    constant OutlineUniforms& uniforms [[ buffer(BufferIndexOutlineUniforms) ]],
                                    texture2d_array<float> mainDepthTexture [[ texture(MainDepthTextureIndex) ]],
                                    texture2d_array<float> mattingDepthTexture [[ texture(MattingDepthIndexInput) ]],
                                    constant rasterization_rate_map_data &rateMapData [[buffer(BufferIndexRasterizationRateMapUniforms)]]
                                    )
{
    rasterization_rate_map_decoder map(rateMapData);
    const float2 screenCoords = in.textCoords * uniforms.viewportSize;
    
    float2 physCoords = map.map_screen_to_physical_coordinates(screenCoords, in.cameraIndex);
    
    constexpr sampler depthSampler(coord::pixel,
                                   address::clamp_to_edge,
                                   filter::linear);
    
    float mattingDepth = mattingDepthTexture.sample(depthSampler, physCoords, in.cameraIndex).r;
    
    FragmentShader_SelectionOutlineOut out;
    
#if 0
    if (mattingDepth != 0.f)
    {
        out.depth = mattingDepth;
        out.color = float4 { 1.f, 0.f, 0.f, 1.f };
    }
    else
    {
        discard_fragment();
    }
    
    return out;
#else
    
    const auto delta = uniforms.samplingDelta;
    
    size_t n = 0;
    
    for (float x = -delta.x; x <= delta.x; x += delta.x)
    {
        for (float y = -delta.y; y <= delta.y; y += delta.y)
        {
            const auto d = mattingDepthTexture.sample(depthSampler, physCoords + float2 { x, y }, in.cameraIndex).r;
            mattingDepth = max(mattingDepth, d);
            
            if (d != 0.f)
            {
                ++n;
            }
        }
    }
    
    if ((n > 0) && (n < 9))
    {
        out.depth = mattingDepth;
        
        out.color = uniforms.color;
        
#if !TARGET_OS_VISION
        const float originalDepth = mainDepthTexture.sample(depthSampler, physCoords).r;
        if (originalDepth >= mattingDepth)
        {
            out.color.a *= 0.25f;
        }
#endif
    }
    else
    {
        discard_fragment();
    }
#endif
    
    return out;
}
