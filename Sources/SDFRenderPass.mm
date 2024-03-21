//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "SDFRenderPass.h"
#include "Renderer.h"

SDFRenderPass::SDFRenderPass() = default;

PipelineConfiguration::Ptr
SDFRenderPass::makePipelineConfiguration(Renderer& renderer) const
{
    auto config = _inherited::makePipelineConfiguration(renderer);
    
    auto presentationConfig = renderer.delegate()->presentConfiguration();
    config->colorPixelFormat = presentationConfig->colorPixelFormat;
    config->depthPixelFormat = presentationConfig->depthPixelFormat;
    
    const auto depthInfo = renderer.delegate()->depthInfo();
    config->depthCompareFunction = depthInfo.compareFunction;
    
    config->pipelineName = "RGB Contents";
    
    auto mtlLib = renderer.mtlLibrary();
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
    
    _viewDependentUniformsBuffer = std::make_unique<ViewDependentUniformsBuffer>(device, @"ViewDependentUniforms");
    _materialsBuffer = std::make_unique<SerializedMaterials>(device, @"Materials");

    return true;
}

void
SDFRenderPass::updateBuffersState()
{
    _viewDependentUniformsBuffer->update();
    _materialsBuffer->update();
}

void
SDFRenderPass::updateUniforms(Renderer& renderer)
{
    const auto& cameras = renderer.cameraRig()->cameras();
    const size_t cameraCount = cameras.size();
    
    for (size_t cameraIndex=0; cameraIndex < cameraCount; ++cameraIndex)
    {
        auto& cameraUniforms = _viewDependentUniformsBuffer->uniform().cameraUniforms[cameraIndex];

        const auto camera = cameras[cameraIndex];
        
        const float4x4 cameraMatrix = (camera != nullptr) ? camera->worldTransform() : float4x4_identity();
        
        cameraUniforms.viewportSize = camera->viewportSize();
        
        cameraUniforms.cameraMatrix = cameraMatrix;
        cameraUniforms.projectionMatrix = camera->projectionMatrix();
        
        cameraUniforms.viewMatrix = inverse(cameraMatrix);
        cameraUniforms.invProjectionMatrix = camera->invProjectionMatrix();
        
        cameraUniforms.rayOriginZInNDC = camera->rayOriginZInNDC();
        cameraUniforms.rayForwardZInNDC = camera->rayForwardPointZInNDC();
        cameraUniforms.rayLength = camera->rayLength();
        
        cameraUniforms.lightDirection = float3 { -1, -1, -1 };
        
        if (auto world = renderer.world())
        {
            auto& serializedWorld = _viewDependentUniformsBuffer->uniform().serializedWorldObject[cameraIndex];
            auto& serializedMaterials = _materialsBuffer->uniform();
            
            const auto viewMatrix = inverse(cameraMatrix);
            const auto viewProjectionMatrix = camera->projectionMatrix() * viewMatrix;
            
            EncodingContext context { world, viewProjectionMatrix, cameraUniforms.viewportSize, renderer.delegate()->tileSize(), serializedWorld };
            configure(context);
            
            context.encode(*world, serializedMaterials);
        }
    }
}

id <MTLRenderCommandEncoder> _Nullable
SDFRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [renderer.delegate()->renderPassDescriptor(kLeftCameraIndex) copy];
    
    if (renderPassDescriptor != nullptr)
    {
        //renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 1, 0);
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
        
        auto depthAttachment = renderPassDescriptor.depthAttachment;
        
        const auto info = renderer.delegate()->depthInfo();
        depthAttachment.clearDepth = info.clearDepth;
        depthAttachment.loadAction = MTLLoadActionDontCare;
        depthAttachment.storeAction = MTLStoreActionStore;
        
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
    const float2 viewportSize = renderer.cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    const auto& serialized = _viewDependentUniformsBuffer->uniform().serializedWorldObject[kLeftCameraIndex];
    const float2 tileGridSize { serialized.numTileColumns, serialized.numTileRows };
    
    _renderStats.setViewportInfo(viewportSize, tileGridSize);
}

void
SDFRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    _viewDependentUniformsBuffer->setFragmentBuffer(encoder);
    _materialsBuffer->setFragmentBuffer(encoder);
    
    _setupViewports(renderer, encoder);

    _inherited::_render(renderer, encoder);
}

void
SDFRenderPass::onCompletedCommandBuffer(Renderer& renderer, float renderDuration)
{
    _renderStats.submitFrameRenderTime(renderDuration);
}
