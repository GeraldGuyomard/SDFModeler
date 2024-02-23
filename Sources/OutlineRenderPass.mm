//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "OutlineRenderPass.h"

void
OutlineRenderPass::configure(EncodingContext& ctx) const
{
    ctx.setOptionFlags(EncodingContext::fRenderSelectedObjectsOnly);
}

id<MTLRenderCommandEncoder>_Nullable
OutlineRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    
    if (_targetTexture == nil)
    {
        const auto size = renderer.renderSize();
        
        const auto colorPixelFormat = renderer.delegate()->configuration()->colorPixelFormat;
        auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorPixelFormat width:NSUInteger(size.x) height:NSUInteger(size.y) mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageRenderTarget;
        
        _targetTexture = [renderer.mtlDevice() newTextureWithDescriptor:textureDescriptor];
    }
    
    renderPassDescriptor.colorAttachments[0].texture = _targetTexture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    
    renderPassDescriptor.renderTargetArrayLength = 1;
    
    return [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
}

PipelineConfiguration::Ptr
OutlineRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = _inherited::makePipelineConfiguration(mtlLib);
    
    config->pipelineName = "Outline Render";
    
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderMatting"];
    config->depthEnabled = false;
    
    return config;
}
