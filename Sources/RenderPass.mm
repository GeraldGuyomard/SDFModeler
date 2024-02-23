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
    
    auto config = renderer.delegate()->configuration();
    
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = [NSString stringWithUTF8String:_pipelineConfiguration->pipelineName.c_str()];
    pipelineStateDescriptor.rasterSampleCount = config->sampleCount;
    pipelineStateDescriptor.vertexFunction = _pipelineConfiguration->vertexFunction;
    pipelineStateDescriptor.fragmentFunction = _pipelineConfiguration->fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _pipelineConfiguration->vertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = config->colorPixelFormat;
    
    if (_pipelineConfiguration->depthEnabled)
    {
        pipelineStateDescriptor.depthAttachmentPixelFormat = config->depthStencilPixelFormat;
        pipelineStateDescriptor.stencilAttachmentPixelFormat = config->depthStencilPixelFormat;
    }
    
    auto device = renderer.mtlDevice();
    
    NSError *error = NULL;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    if (_pipelineConfiguration->depthEnabled)
    {
        MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
        depthStateDesc.depthWriteEnabled = YES;
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
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder pushDebugGroup:@"RayMarch"];
        [renderEncoder setCullMode:MTLCullModeNone];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        if (_pipelineConfiguration->depthEnabled)
        {
            [renderEncoder setDepthStencilState:_depthState];
        }
        
        _render(renderer, renderEncoder);
        
        [renderEncoder popDebugGroup];
        
        [renderEncoder endEncoding];
    }
}
