//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "Renderer.h"
#include "SDFRenderPass.h"

class SelectionMattingRenderPass : public SDFRenderPass
{
public:
    using _inherited = SDFRenderPass;
    
    bool init(Renderer& renderer) override;
    
    id<MTLTexture> _Nullable targetTexture() const { return _targetTexture; }
    
    
private:
    void configure(EncodingContext&) const override;
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    id<MTLTexture> _Nullable _targetTexture = nil;
    MTLRenderPassDescriptor* _Nullable _renderPassDescriptor = nil;
};
