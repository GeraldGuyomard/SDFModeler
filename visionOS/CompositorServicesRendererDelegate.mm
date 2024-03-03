//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "CompositorServicesRendererDelegate.h"


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
    
    _cameraRig = CameraRig::make(renderer->world(), nbCameras);
    for (const auto& camera : _cameraRig->cameras())
    {
        camera->setIntrinsics(std::make_unique<TangentsCameraIntrinsics>());
    }
    
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
        
        auto* intrinsics = static_cast<TangentsCameraIntrinsics*>(camera->intrinsics());
        
        intrinsics->setTangents(_xrDrawable.tangents(i));
        
        const auto depthRange = _xrDrawable.depthRange();
        
        const float nearPlane = depthRange.y;
        intrinsics->setNearZ(nearPlane);
        
        const float farPlane = depthRange.x;
        intrinsics->setFarZ(farPlane);
        
        const MTLViewport viewport =  _xrDrawable.viewport(i);
        const float2 viewportSize { float(viewport.width), float(viewport.height) };
        camera->setViewportSize(viewportSize);
        
        camera->setProjectionMatrix(intrinsics->computeProjectionMatrix(viewportSize, false));
        
        {
            // debug for info
            const auto fovs = camera->intrinsics()->fovRadians(viewportSize);
            const float fovX = fovs.x * (180.f / M_PI);
            const float fovY = fovs.y * (180.f / M_PI);
            int a = 1;
        }
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
