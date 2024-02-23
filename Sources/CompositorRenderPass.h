//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import "QuadBasedRenderPass.h"

class CompositorRenderPass : public QuadBasedRenderPass
{
public:
    using _inherited = QuadBasedRenderPass;
    
    bool init(Renderer& renderer) override;
    
private:
    
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
};
