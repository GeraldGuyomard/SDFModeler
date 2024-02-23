//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import <Metal/Metal.h>
#import "CommonDefinitions.h"

#import "Uniforms.h"
#import "ShaderTypes.h"

#include "Camera.h"

#include <functional>
#include <string>

#include "RenderPass.h"

class Renderer;

class RendererDelegate
{
public:
    using Ptr = std::unique_ptr<RendererDelegate>;
    virtual ~RendererDelegate() = default;
    
    virtual bool init(Renderer*_Nonnull renderer) = 0;
    
    virtual RenderPassConfiguration configuration() const = 0;
    virtual id<MTLDevice> _Nonnull getMTLDevice() const = 0;
    
    virtual float2 renderSize() const = 0;
    virtual float2 renderSizeInPoints() const = 0;
    
    virtual MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const = 0;
    virtual id <MTLDrawable> _Nonnull currentDrawable() const = 0;
    
    virtual void invalidate() = 0;
    virtual void pause() = 0;    
};

class Renderer final
{
public:
    Renderer(RendererDelegate::Ptr);
    ~Renderer();
    
    float2 renderSize() const;
    
    Camera::Ptr camera() const { return _camera; }
    void setCamera(const Camera::Ptr&);
    
    WorldPtr world() const { return _world; }
    void setWorld(const WorldPtr&);
    
    using RenderCallback = std::function<void(Renderer&)>;
    void setRenderCallback(const RenderCallback&);
    
    Ray ray(float2 pixelPosition) const;
    PickResult pick(float2 pixelPosition) const;
    float4 renderPixel(float2 pixelPosition) const;
    
    void invalidate();
    
    id<MTLDevice> _Nonnull mtlDevice() const;
    
    RendererDelegate* _Nullable delegate() const
    {
        return _delegate.get();
    }
    
public:
    void render();
    void updateCameraTransforms();
    void pause();
    
public:
    const float4x4& projectionMatrix() const { return _projectionMatrix; }
    const float4x4& invProjectionMatrix() const { return _invProjectionMatrix; }
    
private:
    
    void init();
    
    void updateBuffersState();
    void updateUniforms();
    
    Camera::Ptr _camera;
    WorldPtr _world;
    
    RendererDelegate::Ptr _delegate;
    
    dispatch_semaphore_t _Nonnull _inFlightSemaphore;
    
    id <MTLCommandQueue> _Nonnull _commandQueue;

    std::vector<RenderPass*> _renderPasses;
    
    std::unique_ptr<class SDFRenderPass> _sdfRenderPass;
    RenderPass::Ptr _outlineRenderPass;

    float4x4 _projectionMatrix;
    float4x4 _invProjectionMatrix;
    
    RenderCallback _renderCallback;
};
