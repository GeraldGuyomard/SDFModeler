//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "CompositorServicesRendererDelegate.h"
#import <Spatial/Spatial.h>

CompositorServicesRendererDelegate::CompositorServicesRendererDelegate(cp_layer_renderer_t layerRenderer, const XRService::Ptr& xrService)
: _layerRenderer(layerRenderer), _xrService(xrService)
{}

CompositorServicesRendererDelegate::~CompositorServicesRendererDelegate()
{
    if (_renderThread.joinable())
    {
        _shouldStopRendering = true;
        _renderThread.join();
    }
}

bool
CompositorServicesRendererDelegate::init(Renderer* renderer)
{
    _renderer = renderer;
    
    #if TARGET_OS_SIMULATOR
        constexpr size_t nbCameras = 1;
    #else
        constexpr size_t nbCameras = 2;
    #endif
    
    _cameraRig = CameraRig::make(renderer->world(), nbCameras, false);
    
    _configuration = std::make_shared<RenderTargetConfiguration>();
    _configuration->colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    _configuration->depthPixelFormat = MTLPixelFormatDepth32Float;
    
    return true;
}

CameraRig::Ptr
CompositorServicesRendererDelegate::cameraRig() const
{
    return _cameraRig;
}

float2
CompositorServicesRendererDelegate::tileSize() const
{
    //return float2 { 4096, 4096 };
    return float2 { 64, 64 };
}

void
CompositorServicesRendererDelegate::startRenderLoop()
{
    _renderThread = std::thread { [this]()
    {
        while (!_shouldStopRendering)
        {
            const auto state = cp_layer_renderer_get_state(_layerRenderer);
            if (state == cp_layer_renderer_state_invalidated)
            {
                _shouldStopRendering = true;
            }
            else if (state == cp_layer_renderer_state_paused)
            {
                cp_layer_renderer_wait_until_running(_layerRenderer);
            }
            else
            {
                ASSERT(state == cp_layer_renderer_state_running);
                _renderer->render();
            }
        }
    }};
}

RenderTargetConfiguration::CPtr
CompositorServicesRendererDelegate::presentConfiguration() const
{
    return _configuration;
}

id<MTLDevice> _Nonnull 
CompositorServicesRendererDelegate::getMTLDevice() const
{
    return MTLCreateSystemDefaultDevice();
}

bool
CompositorServicesRendererDelegate::startRender(Renderer& renderer)
{
    _xrFrame = _xrService->queryNextFrame(_layerRenderer);
    if (!_xrFrame.isValid())
    {
        return false;
    }
    
    // ????
    _xrFrame.startUpdate();
    _xrFrame.endUpdate();
    
    if (!_xrFrame.waitUntilOptimalTime())
    {
        return false;
    }
    
    return true;
}

namespace
{
    float4x4 convert(const simd_double4x4& in)
    {
        float4x4 m;
        
        for (size_t x = 0; x < 4; ++x)
        {
            for (size_t y = 0; y < 4; ++y)
            {
                m.columns[x][y] = (float) in.columns[x][y];
            }
        }
        
        return m;
    }
}

bool
CompositorServicesRendererDelegate::startSubmission()
{
    _xrFrame.startSubmission();
    
    _xrDrawable = _xrService->queryDrawable(_xrFrame);
    if (!_xrDrawable.isValid())
    {
        return false;
    }
    
    const auto& cameras = _cameraRig->cameras();
    const size_t nbViews = _xrDrawable.viewCount();
    ASSERT(nbViews == cameras.size());
    
    const float4x4 worldHeadTransform = _xrService->worldHeadTransform(_xrDrawable);
    
    for (size_t i=0; i < nbViews; ++i)
    {
        auto camera = cameras[i];
        
        const float4x4 localEyeTransform = _xrDrawable.localEyeTransform(i);
        const float4x4 worldCameraTransform = worldHeadTransform * localEyeTransform;
        
        camera->setWorldTransform(worldCameraTransform);
        
        const float4 tangents = _xrDrawable.tangents(i);
        
        const auto depthRange = _xrDrawable.depthRange();
        
        const float nearPlane = depthRange.y;
        //const float farPlane = depthRange.x;
        const float farPlane = 100.f;
        
        const auto projection = SPProjectiveTransform3DMakeFromTangents(tangents[0],
                                                                        tangents[1],
                                                                        tangents[2],
                                                                        tangents[3],
                                                                        nearPlane,
                                                                        farPlane,
                                                                        false);
        
        camera->setProjectionMatrix(convert(projection.matrix));
        
        const MTLViewport viewport =  _xrDrawable.viewport(i);
        
        const float2 viewportSize { float(viewport.width), float(viewport.height) };
        
        camera->setViewportSize(viewportSize);
    }
        
    return true;
}

void
CompositorServicesRendererDelegate::endSubmission()
{
    _xrFrame.endSubmission();
    
    _xrFrame.invalidate();
    _xrDrawable.invalidate();
}

MTLRenderPassDescriptor* _Nullable
CompositorServicesRendererDelegate::renderPassDescriptor(size_t cameraIndex) const
{
    if (!_xrDrawable.isValid())
    {
        return nil;
    }
    
    auto renderPassDescriptor = [MTLRenderPassDescriptor new];
    
    auto colorAttachment = renderPassDescriptor.colorAttachments[0];
    
    colorAttachment.texture = _xrDrawable.colorTexture(cameraIndex);
    
    colorAttachment.loadAction = MTLLoadActionClear;
    colorAttachment.storeAction = MTLStoreActionStore;
    colorAttachment.clearColor = MTLClearColorMake(0, 0, 0, 0);
    //colorAttachment.clearColor = MTLClearColorMake(1, 0, 0, 0);
    
    auto depthAttachment = renderPassDescriptor.depthAttachment;
    
    depthAttachment.texture = _xrDrawable.depthTexture(cameraIndex);
    depthAttachment.loadAction = MTLLoadActionClear;
    depthAttachment.storeAction = MTLStoreActionStore;
    depthAttachment.clearDepth = 0.0;
    
    return renderPassDescriptor;
}

void
CompositorServicesRendererDelegate::presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer)
{
    if (_xrDrawable.isValid())
    {
        _xrDrawable.present(commandBuffer);
    }
}

void
CompositorServicesRendererDelegate::invalidate()
{
    
}

void
CompositorServicesRendererDelegate::pause()
{
    
}
