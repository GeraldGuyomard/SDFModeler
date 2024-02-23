//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "RenderPass.h"
#include <functional>

class BlurRenderPass : public RenderPass
{
public:
    using _inherited = RenderPass;
    
    bool init(Renderer& renderer) override;
    
    using InputTextureProvider = std::function<id<MTLTexture>()>;
    void setInputTextureProvider(const InputTextureProvider&);
    
private:
    
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    void _render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder) override;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    
    InputTextureProvider _inputTextureProvider;
    id<MTLTexture> _Nullable _targetTexture = nil;
    MTLRenderPassDescriptor* _Nullable _renderPassDescriptor = nil;
};
