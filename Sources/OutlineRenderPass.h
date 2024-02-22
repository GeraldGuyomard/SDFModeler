//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import "Renderer.h"

class OutlineRenderPass : public SDFRenderPass
{
public:
    using _inherited = SDFRenderPass;
    
    bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RendererDelegateConfiguration& config) override;
    void prepareRender(Renderer&) override;
    void render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    
private:
    void configure(EncodingContext&) const override;
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration pipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    MTLRenderPassDescriptor* _Nullable _renderPassDescriptor = nil;
    id<MTLTexture> _Nullable _targetTexture = nil;
    MTLPixelFormat _colorPixelFormat = MTLPixelFormatInvalid;
};
