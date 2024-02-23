//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "OutlineRenderPass.h"

bool
OutlineRenderPass::init(Renderer& renderer)
{
    if(!_inherited::init(renderer))
    {
        return false;
    }
    
    _renderPassDescriptor = [MTLRenderPassDescriptor new];
    
    _renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    _renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    _renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    
    return true;
}

void
OutlineRenderPass::configure(EncodingContext& ctx) const
{
    ctx.setOptionFlags(EncodingContext::fRenderSelectedObjectsOnly);
}

id<MTLRenderCommandEncoder>_Nullable
OutlineRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    const auto size = renderer.renderSize();
    if ((size.x <= 0.f) || ((size.y <= 0.f)))
    {
        return nullptr;
    }
    
    if ((_targetTexture.width != size.x) || (_targetTexture.height != size.y))
    {
        const auto colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorPixelFormat width:NSUInteger(size.x) height:NSUInteger(size.y) mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        
        _targetTexture = [renderer.mtlDevice() newTextureWithDescriptor:textureDescriptor];
    }
    
    _renderPassDescriptor.colorAttachments[0].texture = _targetTexture;
    
    return [cmdBuffer renderCommandEncoderWithDescriptor:_renderPassDescriptor];
}

PipelineConfiguration::Ptr
OutlineRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = _inherited::makePipelineConfiguration(mtlLib);
    
    config->pipelineName = "Outline Render";
    
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderMatting"];
    
    config->depthPixelFormat = MTLPixelFormatInvalid;
    
    return config;
}
