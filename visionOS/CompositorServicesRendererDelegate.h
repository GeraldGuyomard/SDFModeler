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
#import <ARKit/ARKit.h>

#include <thread>
#include <atomic>

class CompositorServicesRendererDelegate final : public RendererDelegate
{
public:
    CompositorServicesRendererDelegate(cp_layer_renderer_t _Nonnull layerRenderer);
    ~CompositorServicesRendererDelegate();
    
    RenderTargetConfiguration::CPtr presentConfiguration() const override;
    bool init(Renderer* _Nonnull) override;
    
    id<MTLDevice> _Nonnull getMTLDevice() const override;
    
    bool startRender(Renderer&) override;
    bool startSubmission() override;
    void endSubmission() override;
    
    size_t cameraInfoCount() const override;
    CameraInfo cameraInfo(size_t index, const Camera::Ptr& camera) const override;
    
    MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const override;
    void presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer) override;
    
    void invalidate() override;
    void pause() override;
    
    void startRenderLoop();
    
private:
    const cp_layer_renderer_t _Nonnull _layerRenderer;
    
    std::thread _renderThread;
    
    ar_session_t _Nullable _arSession = nil;
    ar_world_tracking_provider_t _Nullable _worldTracking = nil;
    
    Renderer* _Nullable _renderer = nullptr;
    std::atomic<bool> _shouldStopRendering = { false };
    
    cp_frame_t _Nullable _frame = nil;
    cp_drawable_t _Nullable _drawable = nil;
    ar_device_anchor_t _deviceAnchor = nil;
    
    std::vector<CameraInfo> _cameraInfos;
    
    RenderTargetConfiguration::Ptr _configuration;
};


