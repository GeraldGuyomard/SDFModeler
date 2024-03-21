//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "QuadBasedRenderPass.h"
#include "ViewDependentUniforms.h"
#include "ShaderTypes.h"
#include "RenderStats.h"

class EncodingContext;

enum class RenderStyle
{
    phong,
    cellShaded,
    flat
};

class SDFRenderPass : public QuadBasedRenderPass
{
public:
    using _inherited = QuadBasedRenderPass;
    
    SDFRenderPass();
    
    bool init(Renderer& renderer) override;
    void updateBuffersState() override;
    void updateUniforms(Renderer&) override;
    
    void willStartRender(Renderer& renderer) override;
    void onCompletedCommandBuffer(Renderer& renderer, float renderDuration) override;
    
    const ViewDependentUniforms& viewDependentUniforms() const
    {
        return _viewDependentUniformsBuffer->uniform();
    }

    const Materials& materials() const
    {
        return _materialsBuffer->uniform();
    }
    
    RenderStyle renderStyle() const
    {
        return _renderStyle;
    }
    
    void setRenderStyle(RenderStyle);
    
protected:
    
    PipelineConfiguration::Ptr makePipelineConfiguration(Renderer&) const override;
    id <MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer,  id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    
    void _render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder) override;
    
    virtual void configure(EncodingContext&) const {}
    
private:
    
    using ViewDependentUniformsBuffer = TUniformBuffer<ViewDependentUniforms, BufferIndex::BufferIndexViewDependentUniforms, kMaxBuffersInFlight>;
    std::unique_ptr<ViewDependentUniformsBuffer> _viewDependentUniformsBuffer;
    
    using SerializedMaterials = TUniformBuffer<Materials, BufferIndex::BufferIndexMaterials, kMaxBuffersInFlight>;
    std::unique_ptr<SerializedMaterials> _materialsBuffer;
    
    RenderStats _renderStats;
    
    RenderStyle _renderStyle = RenderStyle::phong;
};

