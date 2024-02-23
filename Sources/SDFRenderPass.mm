//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "SDFRenderPass.h"
#include "Renderer.h"

Vertex s_Vertices[4] = {
    { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
    { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
    { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
    { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
};

PipelineConfiguration::Ptr
SDFRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = _inherited::makePipelineConfiguration(mtlLib);
    
    config->pipelineName = "SDF Render";
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderSDF"];
    
    return config;
}


bool
SDFRenderPass::init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config)
{
   if (!_inherited::init(device, mtlLib, config))
   {
       return false;
   }
    
    _uniformsBuffer = std::make_unique<UniformsBuffer>(device, @"UniformBuffer");
    _serializedWorldBuffer = std::make_unique<SerializedWorldBuffer>(device, @"SerializedSceneBuffer");
    _materialsBuffer = std::make_unique<SerializedMaterials>(device, @"Materials");

    return true;
}

void
SDFRenderPass::updateBuffersState()
{
    _uniformsBuffer->update();
    _serializedWorldBuffer->update();
    _materialsBuffer->update();
}

void
SDFRenderPass::updateUniforms(Renderer& renderer)
{
    auto& uniforms = _uniformsBuffer->uniform();

    const auto camera = renderer.camera();
    
    const float4x4 cameraMatrix = (camera != nullptr) ? camera->worldTransform() : float4x4_identity();
    
    uniforms.viewportSize = renderer.renderSize();
    uniforms.ndcToWorldTransform = cameraMatrix * renderer.invProjectionMatrix();
    uniforms.worldTransformToNdc = inverse(uniforms.ndcToWorldTransform);
    
    uniforms.lightDirection = float3 { -1, -1, -1 };
    
    if (auto world = renderer.world())
    {
        auto& serializedWorld = _serializedWorldBuffer->uniform();
        auto& serializedMaterials = _materialsBuffer->uniform();
        
        const auto viewMatrix = inverse(cameraMatrix);
        const auto viewProjectionMatrix = renderer.projectionMatrix() * viewMatrix;
        
        EncodingContext context { world, viewProjectionMatrix, uniforms.viewportSize, serializedWorld };
        configure(context);
        
        world->encode(context, serializedMaterials);
    }
}

id <MTLRenderCommandEncoder> _Nullable
SDFRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    auto renderPassDescriptor = renderer.delegate()->currentRenderPassDescriptor();
    
    if (renderPassDescriptor != nullptr)
    {
        return [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    }
    else
    {
        return nullptr;
    }
}

void
SDFRenderPass::willStartRender(Renderer& renderer)
{
    const float2 viewportSize = renderer.renderSize();
    
    const auto& serialized = _serializedWorldBuffer->uniform();
    const float2 tileGridSize { serialized.numTileColumns, serialized.numTileRows };
    
    _renderStats.setViewportInfo(viewportSize, tileGridSize);
}

void
SDFRenderPass::_render(id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    _uniformsBuffer->setFragmentBuffer(encoder);
    _serializedWorldBuffer->setFragmentBuffer(encoder);
    _materialsBuffer->setFragmentBuffer(encoder);
    
    _inherited::_render(encoder);
}

void
SDFRenderPass::onCompletedCommandBuffer(Renderer& renderer, float renderDuration)
{
    _renderStats.submitFrameRenderTime(renderDuration);
}
