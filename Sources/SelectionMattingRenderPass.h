//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "Renderer.h"
#include "SDFRenderPass.h"
#include "Object3D.h"
#include <unordered_set>

class SelectionMattingRenderPass : public SDFRenderPass
{
public:
    using _inherited = SDFRenderPass;
    
    SelectionMattingRenderPass();
    ~SelectionMattingRenderPass() override;
    
    bool init(Renderer& renderer) override;
    
    void updateUniforms(Renderer&) override;
    id<MTLTexture> _Nullable targetDepthTexture() const { return _targetDepthTexture; }
    
    void setObjectsToRender(const Object3DSelection&);
    
private:
    void configure(EncodingContext&) const override;
    id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    PipelineConfiguration::Ptr makePipelineConfiguration(Renderer&) const override;
    
    id<MTLTexture> _Nullable _targetDepthTexture = nil;
    MTLRenderPassDescriptor* _Nullable _renderPassDescriptor = nil;
    
    class MattingEncodingDelegate;
    std::unique_ptr<MattingEncodingDelegate> _encodingDelegate;
};
