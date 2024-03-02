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
#include "XRService.h"

#import <CompositorServices/CompositorServices.h>
#import <ARKit/ARKit.h>

#include <thread>
#include <atomic>

class CompositorServicesRendererDelegate final : public RendererDelegate
{
public:
    CompositorServicesRendererDelegate(cp_layer_renderer_t _Nonnull layerRenderer, const XRService::Ptr& xrService);
    ~CompositorServicesRendererDelegate();
    
    RenderTargetConfiguration::CPtr presentConfiguration() const override;
    bool init(Renderer* _Nonnull) override;
    
    id<MTLDevice> _Nonnull getMTLDevice() const override;
    
    bool startRender(Renderer&) override;
    bool startSubmission() override;
    void endSubmission() override;
    
    CameraRig::Ptr cameraRig() const override;
    float2 tileSize() const override;
    
    MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const override;
    void presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer) override;
    
    void invalidate() override;
    void pause() override;
    
    void startRenderLoop();
    
private:
    const cp_layer_renderer_t _Nonnull _layerRenderer;
    const XRService::Ptr _xrService;
    
    std::thread _renderThread;
    
    Renderer* _Nullable _renderer = nullptr;
    std::atomic<bool> _shouldStopRendering = { false };
    
    XRFrame _xrFrame;
    XRDrawable _xrDrawable;
    CameraRig::Ptr _cameraRig;
    
    RenderTargetConfiguration::Ptr _configuration;
};


