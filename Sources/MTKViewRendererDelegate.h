//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#import <MetalKit/MetalKit.h>

#import "CommonDefinitions.h"
#include "Renderer.h"
#include <optional>

@class MTKViewBridge;

class MTKViewRendererDelegate final : public RendererDelegate
{
public:
    MTKViewRendererDelegate(MTKView* _Nonnull);
    ~MTKViewRendererDelegate();
    
    RenderTargetConfiguration::CPtr presentConfiguration() const override;
    bool init(Renderer* _Nonnull) override;
    
    id<MTLDevice> _Nonnull getMTLDevice() const override;
    
    CameraRig::Ptr cameraRig() const override;
    void updateViewportSize() override;
    
    DepthInfo depthInfo() const override;
    
    MTLRenderPassDescriptor* _Nullable renderPassDescriptor(size_t cameraIndex) const override;
    void presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer) override;
    
    void invalidate() override;
    void pause() override;
    
private:
    const __weak MTKView* _Nullable _mtkView;
    MTKViewBridge* _Nonnull _mtkViewBridge = nil;
    
    RenderTargetConfiguration::Ptr _configuration = std::make_shared<RenderTargetConfiguration>();
    CameraRig::Ptr _cameraRig;
    bool _inverseZ = true;
    
    mutable std::optional<float2> _lastUpdatedViewportSize;
};


