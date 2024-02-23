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
#include "OutlineRenderPass.h"

#include "MainViewController.h"


Renderer::Renderer(RendererDelegate::Ptr delegate)
: _delegate(std::move(delegate)),
_inFlightSemaphore(dispatch_semaphore_create(RenderPass::kMaxBuffersInFlight))
{
    _delegate->init(this);
    
    init();
    updateCameraTransforms();
}

Renderer::~Renderer() = default;

float2
Renderer::renderSize() const
{
    return _delegate->renderSize();
}

void
Renderer::init()
{
    _sdfRenderPass = std::make_unique<SDFRenderPass>();
    _outlineRenderPass = std::make_unique<OutlineRenderPass>();
    
    _renderPasses = { _sdfRenderPass.get(), _outlineRenderPass.get() };
    
    const auto device = _delegate->getMTLDevice();
    id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
    const auto config = _delegate->configuration();
    
    for (auto renderPass : _renderPasses)
    {
        renderPass->init(device, defaultLibrary, config);
    }
    
    //_quadVertexBuffer
    _commandQueue = [device newCommandQueue];
}


void
Renderer::updateBuffersState()
{
    for (auto pass : _renderPasses)
    {
        pass->updateBuffersState();
    }
}

void Renderer::setCamera(const Camera::Ptr& cam)
{
    _camera = cam;
    
    if (_camera != nullptr)
    {
        updateCameraTransforms();
    }
}

void
Renderer::updateUniforms()
{
    for (auto pass : _renderPasses)
    {
        pass->updateUniforms(*this);
    }
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
    
    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);
    
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
            pass->onCompletedCommandBuffer(renderFrameTimeInMs);
        }
        
        dispatch_semaphore_signal(block_sema);
    }];
    
    updateBuffersState();
    updateUniforms();
    
    for (auto pass : _renderPasses)
    {
        pass->prepareRender(*this);
    }
    
    for (auto pass : _renderPasses)
    {
        pass->render(*this, commandBuffer);
    }
    
    auto drawable = _delegate->currentDrawable();
    [commandBuffer presentDrawable:drawable];
    
    [commandBuffer commit];
    
    if (_renderCallback != nullptr)
    {
        _renderCallback(*this);
    }
}

void
Renderer::updateCameraTransforms()
{
    if (_camera != nullptr)
    {
        const auto s = _delegate->renderSizeInPoints();
        
        _camera->setViewportSize(s);
        
        _projectionMatrix = _camera->computeProjectionMatrix();
        _invProjectionMatrix = simd_inverse(_projectionMatrix);
    }
}

Ray
Renderer::ray(float2 pixelPosition) const
{
    const auto size = renderSize();
    const auto p = pixelToNDC(size, pixelPosition);
    
    const auto ray = Ray::make(p, _sdfRenderPass->uniforms());
    return ray;
}

PickResult
Renderer::pick(float2 pixelPosition) const
{
    const auto pixel = renderPixel(pixelPosition);
    
    const auto& uniforms = _sdfRenderPass->uniforms();
    const auto& serializedWorld = _sdfRenderPass->serializedWorld();
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = renderSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return ::pickObject(p, uniforms, serializedWorld, materials);
}

float4
Renderer::renderPixel(float2 pixelPosition) const
{
    const auto& uniforms = _sdfRenderPass->uniforms();
    const auto& serializedWorld = _sdfRenderPass->serializedWorld();
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = renderSize();
    
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
