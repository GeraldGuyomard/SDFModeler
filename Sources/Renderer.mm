//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>

#import "Renderer.h"

#import "ShaderTypes.h"

#include "SerializedWorldObject.h"
#include "FragmentShader/PhongShader.h"

#include "Object3D.h"
#include "RenderFunctions.h"

#include "SDFRenderPass.h"
#include "SelectionMattingRenderPass.h"
#include "SelectionOutlineRenderPass.h"

#include "MainViewController.h"


Renderer::Renderer(RendererDelegate::Ptr delegate)
: _delegate(std::move(delegate)),
_inFlightSemaphore(dispatch_semaphore_create(RenderPass::kMaxBuffersInFlight))
{
    _delegate->init(this);
    
    init();
}

Renderer::~Renderer() = default;

void
Renderer::init()
{
    const auto device = _delegate->getMTLDevice();
    
    _commandQueue = [device newCommandQueue];
    _mtlLibrary = [device newDefaultLibrary];
    
    const size_t n = _delegate->cameraInfoCount();
    _renderPassesPerCamera.resize(n);
    
    for (size_t i=0; i < n; ++i)
    {
        auto& rp = _renderPassesPerCamera[i];
        
        rp.sdfRenderPass = std::make_unique<SDFRenderPass>(i);
        _renderPasses.push_back(rp.sdfRenderPass.get());
        
        rp.selectionMattingRenderPass = std::make_unique<SelectionMattingRenderPass>(i);
        _renderPasses.push_back(rp.selectionMattingRenderPass.get());
        
        rp.selectionOutlineRenderPass = std::make_unique<SelectionOutlineRenderPass>(i);
        _renderPasses.push_back(rp.selectionOutlineRenderPass.get());
        
        rp.selectionOutlineRenderPass->setMattingTextureProvider([selectionMattingPass = rp.selectionMattingRenderPass.get()]()
        {
            return selectionMattingPass->targetTexture();
        });
    }
    
    for (auto renderPass : _renderPasses)
    {
        renderPass->init(*this);
    }
}

bool
CameraInfo::isValid() const
{
    return (_viewportSize.x > 0.f) && (_viewportSize.y > 0.f) && (_viewportSizeInPoints.x > 0.f) && (_viewportSizeInPoints.y > 0.f);
}

void
CameraInfo::setViewportSize(const float2& s)
{
    _viewportSize = s;
}

void
CameraInfo::setViewportSizeInPoints(const float2& s)
{
    _viewportSizeInPoints = s;
}

void
CameraInfo::setProjectionMatrix(const float4x4& proj)
{
    _projectionMatrix = proj;
    _invProjectionMatrix = inverse(_projectionMatrix);
}

void Renderer::installCameraRig()
{
    const size_t n = _delegate->cameraInfoCount();
    
    _cameraRig = CameraRig::make(_world, n);
    _world->rootObject()->addChild(_cameraRig);
    
    _cameraInfos.resize(n);
    _renderPassesPerCamera.resize(n);
}

void
Renderer::setRenderCallback(const RenderCallback& cb)
{
    _renderCallback = cb;
}

void
Renderer::render()
{
    /// Per frame updates here
    auto now = HighResClock::now();
    
    if (!_delegate->startRender(*this))
    {
        return;
    }
    
    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);
    
    _delegate->startSubmission();
    
    if (!_cameraInfosValid && !updateCameraTransforms())
    {
        return;
    }
    
    for (auto pass : _renderPasses)
    {
        pass->updateBuffersState();
        pass->updateUniforms(*this);
        
        pass->willStartRender(*this);
    }
    
    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
        auto end = HighResClock::now();
        const auto dT = end - now;
        
        const float renderFrameTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(dT).count();
        
        for (auto pass : _renderPasses)
        {
            pass->onCompletedCommandBuffer(*this, renderFrameTimeInMs);
        }
        
        dispatch_semaphore_signal(block_sema);
    }];
    
    for (auto pass : _renderPasses)
    {
        pass->render(*this, commandBuffer);
    }
    
    auto drawable = _delegate->currentDrawable();
    [commandBuffer presentDrawable:drawable];
    
    [commandBuffer commit];
    
    _delegate->endSubmission();
    
    if (_renderCallback != nullptr)
    {
        _renderCallback(*this);
    }
}

void
Renderer::invalidateCameraTransforms()
{
    _cameraInfosValid = false;
    
    invalidate();
}

bool
Renderer::updateCameraTransforms()
{
    _cameraInfosValid = false;
    
    if (_cameraRig != nullptr)
    {
        size_t index = 0;
        const auto& cameras = _cameraRig->cameras();
        
        for (auto& info : _cameraInfos)
        {
            auto camera = cameras[index];
            
            info = _delegate->cameraInfo(index, camera);
            if (!info.isValid())
            {
                return false;
            }
            
            camera->setViewportSize(info.viewportSizeInPoints());
            
            ++index;
        }
        
        _cameraInfosValid = true;
    }
    
    return _cameraInfosValid;
}

Ray
Renderer::ray(float2 pixelPosition) const
{
    const auto size = _cameraInfos[kLeftCameraIndex].viewportSize();
    const auto p = pixelToNDC(size, pixelPosition);
    
    const auto ray = Ray::make(p, _renderPassesPerCamera[kLeftCameraIndex].sdfRenderPass->uniforms());
    return ray;
}

PickResult
Renderer::pick(float2 pixelPosition) const
{
    const auto pixel = renderPixel(kLeftCameraIndex, pixelPosition);
    
    auto sdfRenderPass = _renderPassesPerCamera[kLeftCameraIndex].sdfRenderPass.get();
    
    const auto& uniforms = sdfRenderPass->uniforms();
    const auto& serializedWorld = sdfRenderPass->serializedWorld();
    const auto& materials = sdfRenderPass->materials();
    
    const auto size = _cameraInfos[kLeftCameraIndex].viewportSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return ::pickObject(p, uniforms, serializedWorld, materials);
}

float4
Renderer::renderPixel(size_t cameraIndex, float2 pixelPosition) const
{
    auto sdfRenderPass = _renderPassesPerCamera[kLeftCameraIndex].sdfRenderPass.get();
    
    const auto& uniforms = sdfRenderPass->uniforms();
    const auto& serializedWorld = sdfRenderPass->serializedWorld();
    const auto& materials = sdfRenderPass->materials();
    
    const auto size = _cameraInfos[cameraIndex].viewportSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return renderDefault(p, uniforms, serializedWorld, materials).color;
}

void
Renderer::setWorld(const WorldPtr& world)
{
    _world = world;
    
    invalidate();
}

void
Renderer::invalidate()
{
    _delegate->invalidate();
}

void
Renderer::pause()
{
    _delegate->pause();
}

id<MTLDevice>
Renderer::mtlDevice() const
{
    return _commandQueue.device;
}
