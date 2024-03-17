//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "RenderPass.h"
#include "Renderer.h"

bool
RenderPass::init(Renderer& renderer)
{
    _pipelineConfiguration = makePipelineConfiguration(renderer);
    if (_pipelineConfiguration == nullptr)
    {
        return false;
    }
    
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
    
    pipelineStateDescriptor.label = [NSString stringWithUTF8String:_pipelineConfiguration->pipelineName.c_str()];
    pipelineStateDescriptor.rasterSampleCount = _pipelineConfiguration->sampleCount;
    pipelineStateDescriptor.vertexFunction = _pipelineConfiguration->vertexFunction;
    pipelineStateDescriptor.fragmentFunction = _pipelineConfiguration->fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _pipelineConfiguration->vertexDescriptor;
    
    auto colorAttachment = pipelineStateDescriptor.colorAttachments[0];
    
    colorAttachment.pixelFormat = _pipelineConfiguration->colorPixelFormat;
    
    if (_pipelineConfiguration->blendEnabled)
    {
        colorAttachment.blendingEnabled = true;
        colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
        colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;
        
        colorAttachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        colorAttachment.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        
        colorAttachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        colorAttachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    }
    else
    {
        colorAttachment.blendingEnabled = false;
    }
    
    pipelineStateDescriptor.depthAttachmentPixelFormat = _pipelineConfiguration->depthPixelFormat;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
    
    const size_t viewCount = renderer.delegate()->cameraRig()->cameras().size();
    
    pipelineStateDescriptor.maxVertexAmplificationCount = viewCount;
    
    auto device = renderer.mtlDevice();
    
    NSError *error = NULL;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    if (_pipelineConfiguration->depthPixelFormat != MTLPixelFormatInvalid)
    {
        ASSERT(_pipelineConfiguration->depthCompareFunction.has_value());
        if (_pipelineConfiguration->depthCompareFunction.has_value())
        {
            MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
            
            depthStateDesc.depthCompareFunction = _pipelineConfiguration->depthCompareFunction.value();
            depthStateDesc.depthWriteEnabled = true;
            
            _depthState = [device newDepthStencilStateWithDescriptor:depthStateDesc];
        }
    }
    
    return true;
}

void
RenderPass::render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    if (!enabled())
    {
        return;
    }
    
    auto renderEncoder = makeRenderEncoder(renderer, cmdBuffer);
    
    if (renderEncoder != nil)
    {
        [renderEncoder setCullMode:MTLCullModeNone];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        if (_pipelineConfiguration->depthPixelFormat != MTLPixelFormatInvalid)
        {
            [renderEncoder setDepthStencilState:_depthState];
        }
        
        _render(renderer, renderEncoder);
        
        [renderEncoder endEncoding];
    }
}

void
RenderPass::enable(bool e)
{
    _enabled = e;
}

void
RenderPass::_setupViewports(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    const auto& cameras = renderer.cameraRig()->cameras();
    const size_t cameraCount = cameras.size();
    MTLViewport vps[cameraCount];
    
    for (size_t cameraIndex = 0; cameraIndex < cameraCount; ++cameraIndex)
    {
        MTLViewport& vp = vps[cameraIndex];
        
        vp.originX = vp.originY = 0;
        auto camera = cameras[cameraIndex];
        
        const auto& vpSize = camera->viewportSize();
        vp.width = vpSize.x;
        vp.height = vpSize.y;
        vp.znear = 0;
        vp.zfar = 1;
    }
    
    [encoder setViewports:vps count:cameraCount];
    
    if (cameraCount > 1)
    {
        MTLVertexAmplificationViewMapping mapping[cameraCount];
        
        for (uint32_t cameraIndex = 0; cameraIndex < cameraCount; ++cameraIndex)
        {
            mapping[cameraIndex].renderTargetArrayIndexOffset = cameraIndex;
            mapping[cameraIndex].viewportArrayIndexOffset = cameraIndex;
        }
        
        [encoder setVertexAmplificationCount:cameraCount viewMappings:mapping];
    }
}
