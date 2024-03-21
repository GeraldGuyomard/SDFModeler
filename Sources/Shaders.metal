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

template <typename TShader> inline FragmentShaderOut fragmentShaderSDF(
                                                VertexShaderOut in,
                                                constant ViewDependentUniforms& viewDependentUniforms,
                                                constant Materials& materials)
{
    CONSTANT auto& cameraUniforms = viewDependentUniforms.cameraUniforms[in.cameraIndex];
    CONSTANT auto& serializedWorld = viewDependentUniforms.serializedWorldObject[in.cameraIndex];
    
    const auto res = render<TShader>(in.viewportNDC,
                         cameraUniforms,
                         serializedWorld,
                         materials);
    
    
    FragmentShaderOut out;
    
    out.color = res.color;
    out.depth = res.adjustedDepth(cameraUniforms.inverseZ());
    
    return out;
}

fragment FragmentShaderOut fragmentShaderSDF_Phong(VertexShaderOut in [[stage_in]],
                               constant ViewDependentUniforms& viewDependentUniforms [[ buffer(BufferIndexViewDependentUniforms) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    return fragmentShaderSDF<PhongShader>(in, viewDependentUniforms, materials);
}

fragment FragmentShaderOut fragmentShaderSDF_CellShaded(VertexShaderOut in [[stage_in]],
                               constant ViewDependentUniforms& viewDependentUniforms [[ buffer(BufferIndexViewDependentUniforms) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    return fragmentShaderSDF<CellShader>(in, viewDependentUniforms, materials);
}

fragment FragmentShaderOut fragmentShaderSDF_Flat(VertexShaderOut in [[stage_in]],
                               constant ViewDependentUniforms& viewDependentUniforms [[ buffer(BufferIndexViewDependentUniforms) ]],
                               constant Materials& materials [[ buffer(BufferIndexMaterials) ]]
                               )
{
    return fragmentShaderSDF<FlatShader>(in, viewDependentUniforms, materials);
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
    
    const auto res = render<MattingShader, true /*write to depth*/>(in.viewportNDC, cameraUniforms, serializedWorld, materials);
    
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

#if TARGET_OS_VISION && !TARGET_OS_SIMULATOR
    #define USE_LAYERED_TEXTURES_AND_RASTERIZATION_RATE_MAP 1
#else
    #define USE_LAYERED_TEXTURES_AND_RASTERIZATION_RATE_MAP 0
#endif

CONSTANT const float2 kOutlineKernel [8] =
{
    { -1.f, -1.f }, { +0.f, -1.f }, { +1.f, -1.f },
    { -1.f, +0.f },                 { +1.f, +0.f },
    { -1.f, +1.f }, { +0.f, +1.f }, { +1.f, +1.f },
    
};

fragment FragmentShader_SelectionOutlineOut fragmentShaderOutline(VertexShader_SelectionOutlineOut in [[stage_in]],
                                    constant OutlineUniforms& uniforms [[ buffer(BufferIndexOutlineUniforms) ]],
                                    texture2d_array<float> mattingDepthTexture [[ texture(MattingDepthIndexInput) ]],
                                                                  
#if USE_LAYERED_TEXTURES_AND_RASTERIZATION_RATE_MAP
                                    texture2d_array<float> mainDepthTexture [[ texture(MainDepthTextureIndex) ]],
                                    constant rasterization_rate_map_data &rateMapData [[buffer(BufferIndexRasterizationRateMapUniforms)]]
#else
                                    texture2d<float> mainDepthTexture [[ texture(MainDepthTextureIndex) ]]
#endif
                                    )
{
    
    
    const float2 screenCoords = in.textCoords * uniforms.viewportSize;
    
    float2 physCoords;
    
#if USE_LAYERED_TEXTURES_AND_RASTERIZATION_RATE_MAP
        rasterization_rate_map_decoder map(rateMapData);
        physCoords = map.map_screen_to_physical_coordinates(screenCoords, in.cameraIndex);
#else
        physCoords = screenCoords;
#endif
    
    constexpr sampler depthSampler(coord::pixel,
                                   address::clamp_to_edge,
                                   filter::linear);
    
    FragmentShader_SelectionOutlineOut out;
    
    const auto delta = uniforms.samplingDelta;
    
    const float centerMattingDepth = mattingDepthTexture.sample(depthSampler, physCoords, in.cameraIndex).r;
    float mattingDepth = centerMattingDepth;
    float n = ceil(mattingDepth);
    
    constexpr size_t kKernelSize = sizeof(kOutlineKernel) / sizeof(kOutlineKernel[0]);
    
    for(size_t i=0; i < kKernelSize; ++i)
    {
        const float2 c = physCoords + (kOutlineKernel[i] * delta);
        const float d = mattingDepthTexture.sample(depthSampler, c, in.cameraIndex).r;
        mattingDepth = max(mattingDepth, d);
        n += ceil(d);
    }
    
    if ((n > 0) && (n < (kKernelSize + 1)))
    {
        out.depth = mattingDepth;
        
        out.color = uniforms.color;
        
#if !USE_LAYERED_TEXTURES_AND_RASTERIZATION_RATE_MAP
        if (centerMattingDepth >= mattingDepth)
        {
            out.color.a *= 0.25f;
        }
#endif
    }
    else
    {
        discard_fragment();
    }
    
    return out;
}

struct VertexShader_WorkingPlaneOut
{
    float4 position [[position]];
    float2 textCoords;
};


vertex VertexShader_WorkingPlaneOut vertexShader_WorkingPlane(
            VertexShader_WorkingPlaneIn in [[stage_in]],
            constant WorkingPlaneUniforms& uniforms [[ buffer(BufferIndexWorkingPlaneUniform) ]],
            ushort cameraIndex [[amplification_id]])
{
    VertexShader_WorkingPlaneOut out;

    out.position = uniforms.projViewModelMatrix[cameraIndex] * in.position;
    out.textCoords = in.textCoords;

    return out;
}

struct FragmentShader_WorkingPlaneOut
{
    float4 color [[color(0)]];
};

fragment FragmentShader_WorkingPlaneOut fragmentShader_WorkingPlane(VertexShader_WorkingPlaneOut in [[stage_in]],
                                                                    constant WorkingPlaneUniforms& uniforms [[ buffer(BufferIndexWorkingPlaneUniform) ]])
{
    FragmentShader_WorkingPlaneOut out;
    out.color = uniforms.gridColor;
    
    float2 xy = in.textCoords;
    
    xy = fract(xy / uniforms.gridSize);
    xy = step(uniforms.gridThickness, xy);
    
    float pixelVisible = min(1.f, xy.x + xy.y);
    float4 c = uniforms.gridColor * pixelVisible;
    
    // Fade to black (Fog)
    const float fogRatio = in.position.z;
    c.a *= fogRatio;
    
    out.color = c;
    
    return out;
}
