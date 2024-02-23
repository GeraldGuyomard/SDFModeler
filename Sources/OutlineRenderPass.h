//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "Renderer.h"
#include "SDFRenderPass.h"

class OutlineRenderPass : public SDFRenderPass
{
public:
    using _inherited = SDFRenderPass;
    
    bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config) override;
    
private:
    void configure(EncodingContext&) const override;
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    MTLRenderPassDescriptor* _Nullable _renderPassDescriptor = nil;
    id<MTLTexture> _Nullable _targetTexture = nil;
    MTLPixelFormat _colorPixelFormat = MTLPixelFormatInvalid;
};
