//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "SelectionMattingRenderPass.h"

SelectionMattingRenderPass::SelectionMattingRenderPass(size_t cameraIndex)
: _inherited(cameraIndex), _cameraIndex(cameraIndex)
{}

bool
SelectionMattingRenderPass::init(Renderer& renderer)
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
SelectionMattingRenderPass::configure(EncodingContext& ctx) const
{
    ctx.setOptionFlags(EncodingContext::fRenderSelectedObjectsOnly);
}

id<MTLRenderCommandEncoder>_Nullable
SelectionMattingRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    auto size = renderer.cameraInfos()[_cameraIndex].viewportSize();
    if ((size.x <= 0.f) || ((size.y <= 0.f)))
    {
        return nullptr;
    }
    
    // render at a lower resolution than final content
    // to save time and get free blur
    //size = ceil(size * 0.75f);
    
    if ((_targetTexture.width != size.x) || (_targetTexture.height != size.y))
    {
        const auto colorPixelFormat = pipelineConfiguration()->colorPixelFormat;
        auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorPixelFormat width:NSUInteger(size.x) height:NSUInteger(size.y) mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        
        _targetTexture = [renderer.mtlDevice() newTextureWithDescriptor:textureDescriptor];
    }
    
    _renderPassDescriptor.colorAttachments[0].texture = _targetTexture;
    
    auto encoder = [cmdBuffer renderCommandEncoderWithDescriptor:_renderPassDescriptor];
    encoder.label = @"SelectionMattingRenderPass";
    return encoder;
}

PipelineConfiguration::Ptr
SelectionMattingRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = _inherited::makePipelineConfiguration(mtlLib);
    
    config->pipelineName = "Selection Matting";
    
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderMatting"];
    
    config->depthPixelFormat = MTLPixelFormatInvalid;
    
    return config;
}
