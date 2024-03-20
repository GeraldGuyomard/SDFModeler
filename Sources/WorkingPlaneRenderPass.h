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

class WorkingPlaneRenderPass : public RenderPass
{
public:
    using _inherited = RenderPass;
    
    WorkingPlaneRenderPass();
    
    bool init(Renderer& renderer) override;
    
    void updateBuffersState() override;
    void updateUniforms(Renderer&) override;
    
private:
    
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(Renderer&) const override;
    
    void _render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder) override;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    
    using UniformsBuffer = TUniformBuffer<WorkingPlaneUniforms, BufferIndexWorkingPlaneUniform, kMaxBuffersInFlight>;
    std::unique_ptr<UniformsBuffer> _uniformsBuffer;
    
    float4x4 _gridTransform;
};
