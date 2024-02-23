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
    if (_targetTexture == nullptr)
    {
        const auto size = renderer.renderSize();
        if ((size.x <= 0.f) || ((size.y <= 0.f)))
        {
            return nullptr;
        }
        
        const auto colorPixelFormat = renderer.delegate()->configuration()->colorPixelFormat;
        auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorPixelFormat width:NSUInteger(size.x) height:NSUInteger(size.y) mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        
        _targetTexture = [renderer.mtlDevice() newTextureWithDescriptor:textureDescriptor];
    }
    
    MTLRenderPassDescriptor* renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    
    renderPassDescriptor.colorAttachments[0].texture = _targetTexture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    
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

void
OutlineRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    _inherited::_render(renderer, encoder);
}
