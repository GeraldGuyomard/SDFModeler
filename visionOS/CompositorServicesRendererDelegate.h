//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#import "CommonDefinitions.h"
#include "Renderer.h"
#include "RenderTargetConfiguration.h"

#import <CompositorServices/CompositorServices.h>

class CompositorServicesRendererDelegate final : public RendererDelegate
{
public:
    CompositorServicesRendererDelegate(cp_layer_renderer_t _Nonnull layer_renderer);
    ~CompositorServicesRendererDelegate();
    
    RenderTargetConfiguration::CPtr configuration() const override;
    bool init(Renderer* _Nonnull) override;
    
    id<MTLDevice> _Nonnull getMTLDevice() const override;
    
    float2 renderSize() const override;
    float2 renderSizeInPoints() const override;
    
    MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const override;
    id <MTLDrawable> _Nonnull currentDrawable() const override;
    
    void invalidate() override;
    void pause() override;
    
private:
    cp_layer_renderer_t _Nonnull _layerRenderer;
    RenderTargetConfiguration::CPtr _configuration = std::make_shared<RenderTargetConfiguration>();
};


