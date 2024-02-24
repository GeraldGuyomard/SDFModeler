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

@class MTKViewBridge;

class MTKViewRendererDelegate final : public RendererDelegate
{
public:
    MTKViewRendererDelegate(MTKView* _Nonnull);
    ~MTKViewRendererDelegate();
    
    RenderTargetConfiguration::CPtr presentConfiguration() const override;
    bool init(Renderer* _Nonnull) override;
    
    id<MTLDevice> _Nonnull getMTLDevice() const override;
    
    size_t cameraInfoCount() const override;
    CameraInfo cameraInfo(size_t index, const Camera::Ptr& camera) const override;
    
    MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const override;
    id <MTLDrawable> _Nonnull currentDrawable() const override;
    
    void invalidate() override;
    void pause() override;
    
private:
    const __weak MTKView* _Nullable _mtkView;
    MTKViewBridge* _Nonnull _mtkViewBridge = nil;
    
    RenderTargetConfiguration::Ptr _configuration = std::make_shared<RenderTargetConfiguration>();
};


