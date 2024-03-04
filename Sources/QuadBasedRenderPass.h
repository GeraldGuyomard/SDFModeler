//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "RenderPass.h"
#include "ShaderTypes.h"
#include "TUniformBuffer.h"

class EncodingContext;

class QuadBasedRenderPass : public RenderPass
{
public:
    using _inherited = RenderPass;
    
    bool init(Renderer& renderer) override;
    
protected:
    
    PipelineConfiguration::Ptr makePipelineConfiguration(Renderer&) const override;
    
    void _render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder) override;
    
private:
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
};

