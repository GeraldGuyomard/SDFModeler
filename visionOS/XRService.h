//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"
#import <ARKit/ARKit.h>
#import <CompositorServices/CompositorServices.h>

#include <memory>
#include <functional>

@class XRServiceImpl, XRFrameImpl, XRDrawableImpl;

class XRFrame final
{
public:
    
    XRFrame() = default;
    XRFrame(XRFrameImpl*_Nullable);
    ~XRFrame();
    
    void startUpdate();
    void endUpdate();
    
    bool waitUntilOptimalTime();
    
    void startSubmission();
    void endSubmission();
    
    bool isValid() const
    {
        return _impl != nullptr;
    }
    
    XRFrameImpl* _Nullable impl() const
    {
        return _impl;
    }
    
    void invalidate()
    {
        _impl = nil;
    }
    
private:
    XRFrameImpl* _Nonnull _impl = nil;
};

class XRDrawable final
{
public:
    using Ptr = std::unique_ptr<XRDrawable>;
    
    XRDrawable() = default;
    XRDrawable(XRDrawableImpl*_Nullable);
    ~XRDrawable();
    
    bool isValid() const
    {
        return _impl != nullptr;
    }
    
    XRDrawableImpl* _Nullable impl() const
    {
        return _impl;
    }
    
    size_t viewCount() const;
    float4x4 localEyeTransform(size_t index) const;
    float4 tangents(size_t index) const;
    MTLViewport viewport(size_t index) const;
    
    float2 depthRange() const;
    
    id<MTLTexture> colorTexture() const;
    id<MTLTexture> depthTexture() const;
    
    void present(id<MTLCommandBuffer> cmdBuffer);
    
    void invalidate()
    {
        _impl = nil;
    }
    
private:
    XRDrawableImpl* _Nonnull _impl = nil;
};

class XRService final
{
public:
    using Ptr = std::shared_ptr<XRService>;

    static Ptr make();
    ~XRService();

    using Completion = std::function<void()>;
    void start(const Completion&);
    
    XRFrame queryNextFrame(cp_layer_renderer_t _Nonnull layerRenderer);
    
    XRDrawable queryDrawable(const XRFrame& frame);
    
    float4x4 worldHeadTransform(const XRDrawable&) const;
    
private:
    XRService();
    bool _init();
    
    XRServiceImpl* _Nonnull _impl = nil;
};


