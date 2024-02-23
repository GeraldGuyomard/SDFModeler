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
    
    bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderTargetConfiguration::CPtr& config) override;
    
protected:
    
    PipelineConfiguration::Ptr makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    
    void _render(id<MTLRenderCommandEncoder> _Nonnull encoder) override;
    
private:
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
};

