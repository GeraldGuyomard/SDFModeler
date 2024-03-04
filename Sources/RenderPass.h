//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import <Metal/Metal.h>
#include <memory>
#include <string>
#include "RenderTargetConfiguration.h"

class Renderer;

struct PipelineConfiguration final : public RenderTargetConfiguration
{
    using Ptr = std::unique_ptr<PipelineConfiguration>;
    using CPtr = std::unique_ptr<const PipelineConfiguration>;
    
    MTLVertexDescriptor* _Nonnull vertexDescriptor = nil;
    id <MTLFunction> _Nonnull vertexFunction = nil;
    
    id <MTLFunction> _Nonnull fragmentFunction = nil;
    
    bool blendEnabled = false;
    
    std::optional<MTLCompareFunction> depthCompareFunction; //MTLCompareFunctionLessEqual;
    
    std::string pipelineName;
};

class RenderPass
{
public:
    using Ptr = std::unique_ptr<RenderPass>;
    
    virtual ~RenderPass() = default;
    
    virtual bool init(Renderer& renderer) = 0;
    
    virtual void updateBuffersState() {}
    virtual void updateUniforms(Renderer&) {}
    
    void render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer);
    
    virtual void willStartRender(Renderer&) {}
    virtual void onCompletedCommandBuffer(Renderer& renderer, float renderDuration) {}
    
    static constexpr size_t kMaxBuffersInFlight = 3;
    
    const PipelineConfiguration* pipelineConfiguration() const { return _pipelineConfiguration.get(); }
    
    bool enabled() const { return _enabled; }
    void enable(bool);
    
protected:
    
    virtual PipelineConfiguration::Ptr makePipelineConfiguration(Renderer&) const = 0;
    virtual id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer,  id<MTLCommandBuffer> _Nonnull cmdBuffer) = 0;
    virtual void _render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder) = 0;
    
private:
    PipelineConfiguration::Ptr _pipelineConfiguration;
    
    id <MTLRenderPipelineState> _Nonnull _pipelineState;
    id <MTLDepthStencilState> _Nullable _depthState;
    
    bool _enabled = true;
};
