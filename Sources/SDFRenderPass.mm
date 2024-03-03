//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "SDFRenderPass.h"
#include "Renderer.h"

SDFRenderPass::SDFRenderPass(size_t cameraIndex)
: _cameraIndex(cameraIndex)
{}

PipelineConfiguration::Ptr
SDFRenderPass::makePipelineConfiguration(const RenderTargetConfiguration::CPtr& presentationConfig, id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = _inherited::makePipelineConfiguration(presentationConfig, mtlLib);
    
    config->colorPixelFormat = presentationConfig->colorPixelFormat;
    config->depthPixelFormat = presentationConfig->depthPixelFormat;
    
    config->pipelineName = "RGB Contents";
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderSDF"];
    
    return config;
}


bool
SDFRenderPass::init(Renderer& renderer)
{
   if (!_inherited::init(renderer))
   {
       return false;
   }
    
    auto device = renderer.mtlDevice();
    
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

    const auto camera = renderer.cameraRig()->cameras()[_cameraIndex];
    
    const float4x4 cameraMatrix = (camera != nullptr) ? camera->worldTransform() : float4x4_identity();
    
    uniforms.viewportSize = camera->viewportSize();
    
    uniforms.worldTransformToNdc = camera->projectionMatrix() * inverse(cameraMatrix);
    uniforms.ndcToWorldTransform = inverse(uniforms.worldTransformToNdc);
    
    uniforms.nearZInNDC = camera->nearZInNDC();
    uniforms.farZInNDC = camera->farZInNDC();
    uniforms.rayLength = camera->rayLength();
    
    uniforms.lightDirection = float3 { -1, -1, -1 };
    
    if (auto world = renderer.world())
    {
        auto& serializedWorld = _serializedWorldBuffer->uniform();
        auto& serializedMaterials = _materialsBuffer->uniform();
        
        const auto viewMatrix = inverse(cameraMatrix);
        const auto viewProjectionMatrix = camera->projectionMatrix() * viewMatrix;
        
        EncodingContext context { world, viewProjectionMatrix, uniforms.viewportSize, renderer.delegate()->tileSize(), serializedWorld };
        configure(context);
        
        world->encode(context, serializedMaterials);
    }
}

id <MTLRenderCommandEncoder> _Nullable
SDFRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [renderer.delegate()->renderPassDescriptor(_cameraIndex) copy];
    
    if (renderPassDescriptor != nullptr)
    {
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(1, 0, 1, 0);
        
        auto encoder = [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        encoder.label = @"SDFRenderPass";
        return encoder;
    }
    else
    {
        return nullptr;
    }
}

void
SDFRenderPass::willStartRender(Renderer& renderer)
{
    const float2 viewportSize = renderer.cameraRig()->cameras()[_cameraIndex]->viewportSize();
    
    const auto& serialized = _serializedWorldBuffer->uniform();
    const float2 tileGridSize { serialized.numTileColumns, serialized.numTileRows };
    
    _renderStats.setViewportInfo(viewportSize, tileGridSize);
}

void
SDFRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    _uniformsBuffer->setFragmentBuffer(encoder);
    _serializedWorldBuffer->setFragmentBuffer(encoder);
    _materialsBuffer->setFragmentBuffer(encoder);
    
    MTLViewport vp;
    vp.originX = vp.originY = 0;
    auto camera = renderer.cameraRig()->cameras()[_cameraIndex];
    
    const auto& vpSize = camera->viewportSize();
    vp.width = vpSize.x;
    vp.height = vpSize.y;
    vp.znear = 0;
    vp.zfar = 1;
    
    [encoder setViewport:vp];
    
    _inherited::_render(renderer, encoder);
}

void
SDFRenderPass::onCompletedCommandBuffer(Renderer& renderer, float renderDuration)
{
    _renderStats.submitFrameRenderTime(renderDuration);
}
