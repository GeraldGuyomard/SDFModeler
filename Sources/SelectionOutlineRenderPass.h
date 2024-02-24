//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "RenderPass.h"
#include <functional>
#include "TUniformBuffer.h"

class SelectionOutlineRenderPass : public RenderPass
{
public:
    using _inherited = RenderPass;
    
    bool init(Renderer& renderer) override;
    
    using MattingTextureProvider = std::function<id<MTLTexture>()>;
    void setMattingTextureProvider(const MattingTextureProvider&);
    
    void updateBuffersState() override;
    void updateUniforms(Renderer&) override;
    
private:
    
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    void _render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder) override;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    
    using UniformsBuffer = TUniformBuffer<OutlineUniforms, BufferIndexMattingUniforms, kMaxBuffersInFlight>;
    std::unique_ptr<UniformsBuffer> _uniformsBuffer;
    
    MattingTextureProvider _mattingTextureProvider;
};
