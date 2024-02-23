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
    _pipelineConfiguration = makePipelineConfiguration(renderer.mtlLibrary());
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
    
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = _pipelineConfiguration->colorPixelFormat;
    
    if (_pipelineConfiguration->blendEnabled)
    {
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    }
    else
    {
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;
    }
    
    pipelineStateDescriptor.depthAttachmentPixelFormat = _pipelineConfiguration->depthPixelFormat;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = _pipelineConfiguration->depthPixelFormat;
    
    auto device = renderer.mtlDevice();
    
    NSError *error = NULL;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    if (_pipelineConfiguration->depthPixelFormat != MTLPixelFormatInvalid)
    {
        MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
        depthStateDesc.depthWriteEnabled = _pipelineConfiguration->depthWriteEnabled;
        _depthState = [device newDepthStencilStateWithDescriptor:depthStateDesc];
    }
    
    return true;
}

void
RenderPass::render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
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
