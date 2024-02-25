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

#include "CameraRig.h"

#include <functional>
#include <string>

#include "RenderTargetConfiguration.h"

class Renderer;
class RenderPass;

class CameraInfo final
{
public:
    CameraInfo() = default;
    
    bool isValid() const;
    
    const float2& viewportSize() const { return _viewportSize; }
    const float2& viewportSizeInPoints() const { return _viewportSizeInPoints; }
    
    const float4x4& projectionMatrix() const { return _projectionMatrix; }
    const float4x4& invProjectionMatrix() const { return _invProjectionMatrix; }
    
    void setViewportSize(const float2&);
    void setViewportSizeInPoints(const float2&);
    void setProjectionMatrix(const float4x4&);
    
private:
    float2 _viewportSize = { 0.f };
    float2 _viewportSizeInPoints = { 0.f };
    
    float4x4 _projectionMatrix = float4x4_identity();
    float4x4 _invProjectionMatrix = float4x4_identity();
};

class RendererDelegate
{
public:
    using Ptr = std::unique_ptr<RendererDelegate>;
    virtual ~RendererDelegate() = default;
    
    virtual bool init(Renderer*_Nonnull renderer) = 0;
    
    virtual RenderTargetConfiguration::CPtr presentConfiguration() const = 0;
    virtual id<MTLDevice> _Nonnull getMTLDevice() const = 0;
    
    virtual bool startRender(Renderer&) { return true; }
    virtual void startSubmission() {}
    virtual void endSubmission() {}
    
    virtual size_t cameraInfoCount() const = 0;
    virtual CameraInfo cameraInfo(size_t index, const Camera::Ptr& camera) const = 0;
    
    virtual MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const = 0;
    virtual void presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer) = 0;
    
    virtual void invalidate() = 0;
    virtual void pause() = 0;    
};

class Renderer final
{
public:
    Renderer(RendererDelegate::Ptr);
    ~Renderer();
    
    CameraRig::Ptr cameraRig() const { return _cameraRig; }
    void installCameraRig();
    
    WorldPtr world() const { return _world; }
    void setWorld(const WorldPtr&);
    
    using RenderCallback = std::function<void(Renderer&)>;
    void setRenderCallback(const RenderCallback&);
    
    const std::vector<CameraInfo>& cameraInfos() const { return _cameraInfos; }
    
    Ray ray(float2 pixelPosition) const;
    PickResult pick(float2 pixelPosition) const;
    float4 renderPixel(size_t cameraIndex, float2 pixelPosition) const;
    
    void invalidate();
    
    id<MTLDevice> _Nonnull mtlDevice() const;
    id<MTLLibrary> _Nonnull mtlLibrary() const { return _mtlLibrary; }
    
    RendererDelegate* _Nullable delegate() const
    {
        return _delegate.get();
    }
    
public:
    void render();
    void invalidateCameraTransforms();
    void pause();
    
private:
    
    void init();
    bool updateCameraTransforms();
    
    WorldPtr _world;
    
    RendererDelegate::Ptr _delegate;
    
    dispatch_semaphore_t _Nonnull _inFlightSemaphore;
    
    id<MTLLibrary> _Nullable _mtlLibrary;
    
    id <MTLCommandQueue> _Nonnull _commandQueue;

    std::vector<RenderPass*> _renderPasses;
    
    struct RenderPassesPerCamera final
    {
        std::unique_ptr<class SDFRenderPass> sdfRenderPass;
        std::unique_ptr<class SelectionMattingRenderPass> selectionMattingRenderPass;
        std::unique_ptr<class SelectionOutlineRenderPass> selectionOutlineRenderPass;
    };
    
    CameraRig::Ptr _cameraRig;
    std::vector<CameraInfo> _cameraInfos;
    std::vector<RenderPassesPerCamera> _renderPassesPerCamera;
    bool _cameraInfosValid = false;
    
    RenderCallback _renderCallback;
};
