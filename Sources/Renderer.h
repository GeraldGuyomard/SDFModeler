//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import <Metal/Metal.h>
#import "CommonDefinitions.h"

#import "CameraUniforms.h"
#import "ShaderTypes.h"

#include "CameraRig.h"

#include <functional>
#include <string>

#include "RenderTargetConfiguration.h"
#include <TargetConditionals.h>

class Renderer;
class RenderPass;

class RendererDelegate
{
public:
    using Ptr = std::unique_ptr<RendererDelegate>;
    virtual ~RendererDelegate() = default;
    
    virtual bool init(Renderer*_Nonnull renderer) = 0;
    
    virtual RenderTargetConfiguration::CPtr presentConfiguration() const = 0;
    virtual id<MTLDevice> _Nonnull getMTLDevice() const = 0;
    
    virtual void updateViewportSize() {}
    
    virtual bool startRender(Renderer&) { return true; }
    virtual bool startSubmission() { return true; }
    virtual void endSubmission() {}
    
    virtual CameraRig::Ptr cameraRig() const = 0;
    virtual float2 tileSize() const;
    
    struct DepthInfo
    {
        float clearDepth;
        MTLCompareFunction compareFunction;
        
        DepthInfo(float clearDepth, MTLCompareFunction compareFunction);
    };
    
    virtual DepthInfo depthInfo() const = 0;
    
    virtual float contentScaleFactor() const { return 1.f; }
    
    virtual MTLRenderPassDescriptor* _Nullable renderPassDescriptor(size_t cameraIndex) const = 0;
    virtual void presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer) = 0;
    
    virtual void invalidate() = 0;
    virtual void pause() = 0;    
};

#if TARGET_OS_OSX
    typedef NSImage AppleImage;
#else
    typedef UIImage AppleImage;
#endif

class Renderer final
{
public:
    Renderer(const WorldPtr& world, RendererDelegate::Ptr);
    ~Renderer();
    
    CameraRig::Ptr cameraRig() const { return _delegate->cameraRig(); }
    
    WorldPtr world() const { return _world; }
    
    using RenderCallback = std::function<void(Renderer&)>;
    void setRenderCallback(const RenderCallback&);
    
    Ray ray(float2 pixelPosition) const;
    PickResult pick(float2 pixelPosition) const;
    float4 renderPixel(size_t cameraIndex, float2 pixelPosition) const;
    float renderMatting(size_t cameraIndex, float2 pixelPosition) const;
    
    void invalidate();
    
    id<MTLDevice> _Nonnull mtlDevice() const;
    id<MTLLibrary> _Nonnull mtlLibrary() const { return _mtlLibrary; }
    
    RendererDelegate* _Nullable delegate() const
    {
        return _delegate.get();
    }
    
    const std::vector<RenderPass*>& renderPasses() const { return _renderPasses; }
    
    AppleImage* _Nonnull renderImage() const;
    
    dispatch_semaphore_t _Nonnull inFlightSemaphore() const { return _inFlightSemaphore; }
    
public:
    void render();
    void pause();
    
private:
    
    void init();
    
    WorldPtr _world;
    
    RendererDelegate::Ptr _delegate;
    
    dispatch_semaphore_t _Nonnull _inFlightSemaphore;
    
    id<MTLLibrary> _Nullable _mtlLibrary;
    
    id <MTLCommandQueue> _Nonnull _commandQueue;

    std::vector<RenderPass*> _renderPasses;
    
    std::unique_ptr<class SDFRenderPass> _sdfRenderPass;
    std::unique_ptr<class SelectionMattingRenderPass> _selectionMattingRenderPass;
    std::unique_ptr<class SelectionOutlineRenderPass> _selectionOutlineRenderPass;
    std::unique_ptr<RenderPass> _workingPlaneRenderPass;
    
    RenderCallback _renderCallback;
};
