//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import "Renderer.h"

class CompositorRenderPass : public RenderPass
{
public:
    using _inherited = RenderPass;
    
    bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config) override;
    void prepareRender(Renderer&) override;
    void render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    
private:
    
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration pipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    MTLRenderPassDescriptor* _Nullable _renderPassDescriptor = nil;
    id<MTLTexture> _Nullable _targetTexture = nil;
    MTLPixelFormat _colorPixelFormat = MTLPixelFormatInvalid;
};
