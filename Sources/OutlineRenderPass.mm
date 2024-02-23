//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "OutlineRenderPass.h"

bool
OutlineRenderPass::init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config)
{
    if (!_inherited::init(device, mtlLib, config))
    {
        return false;
    }
    
    _colorPixelFormat = config.colorPixelFormat;
    
    _renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    
    auto colorAttachmentDescr = [[MTLRenderPassColorAttachmentDescriptor alloc] init];
    colorAttachmentDescr.clearColor = MTLClearColorMake(0, 0, 0, 0);
    
    [_renderPassDescriptor.colorAttachments setObject:colorAttachmentDescr atIndexedSubscript:0];
    
    return true;
}

void
OutlineRenderPass::configure(EncodingContext& ctx) const
{
    ctx.setOptionFlags(EncodingContext::fRenderSelectedObjectsOnly);
}

void
OutlineRenderPass::prepareRender(Renderer& renderer)
{
    if (_targetTexture == nil)
    {
        const auto size = renderer.renderSize();
        
        auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:_colorPixelFormat width:NSUInteger(size.x) height:NSUInteger(size.y) mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageRenderTarget;
        
        _targetTexture = [renderer.mtlDevice() newTextureWithDescriptor:textureDescriptor];
    }
}

id<MTLRenderCommandEncoder>_Nullable
OutlineRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    
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
