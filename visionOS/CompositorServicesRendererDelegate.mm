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
     _frame = cp_layer_renderer_query_next_frame(_layerRenderer);
    if (_frame == nullptr)
    {
        return false;
    }
    
    // ????
    cp_frame_start_update(_frame);
    cp_frame_end_update(_frame);
    
    cp_frame_timing_t timing = cp_frame_predict_timing(_frame);
    if (timing == nullptr)
    {
        return false;
    }
    
    const cp_time_t time = cp_frame_timing_get_optimal_input_time(timing);
    cp_time_wait_until(time);
    
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
    cp_frame_start_submission(_frame);
    
    cp_frame_timing_t timing = cp_frame_predict_timing(_frame);
    if (timing == nullptr)
    {
        return false;
    }
    
    const cp_time_t t = cp_frame_timing_get_presentation_time(timing);
    const CFTimeInterval timeStamp = cp_time_to_cf_time_interval(t);
    
    const auto deviceAnchor = _xrService->queryDeviceAnchor(timeStamp);
    if (deviceAnchor == nullptr)
    {
        return false;
    }
    
    _drawable = cp_frame_query_drawable(_frame);
    if (_drawable == nullptr)
    {
        return false;
    }
    
    const auto& cameras = _cameraRig->cameras();
    const size_t nbViews = cp_drawable_get_view_count(_drawable);
    ASSERT(nbViews == cameras.size());
    
    cp_drawable_set_device_anchor(_drawable, deviceAnchor);
    
    const float4x4 worldHeadTransform = ar_anchor_get_origin_from_anchor_transform(deviceAnchor);
    
    for (size_t i=0; i < nbViews; ++i)
    {
        auto camera = cameras[i];
        const cp_view_t view = cp_drawable_get_view(_drawable, i);
        
        const float4x4 localEyeTransform = cp_view_get_transform(view);
        const float4x4 worldCameraTransform = worldHeadTransform * localEyeTransform;
        
        camera->setWorldTransform(worldCameraTransform);
        
        const float4 tangents = cp_view_get_tangents(view);
        
        const auto depthRange = cp_drawable_get_depth_range(_drawable);
        
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
        
        const auto textureMap = cp_view_get_view_texture_map(view);
        const MTLViewport viewport =  cp_view_texture_map_get_viewport(textureMap);
        
        const float2 viewportSize { float(viewport.width), float(viewport.height) };
        
        camera->setViewportSize(viewportSize);
    }
        
    return true;
}

void
CompositorServicesRendererDelegate::endSubmission()
{
    cp_frame_end_submission(_frame);
    
    _frame = nil;
    _drawable = nil;
}

MTLRenderPassDescriptor* _Nullable
CompositorServicesRendererDelegate::currentRenderPassDescriptor() const
{
    if (_drawable == nil)
    {
        return nil;
    }
    
    auto renderPassDescriptor = [MTLRenderPassDescriptor new];
    
    auto colorAttachment = renderPassDescriptor.colorAttachments[0];
    
    colorAttachment.texture = cp_drawable_get_color_texture(_drawable, 0);
    
    colorAttachment.loadAction = MTLLoadActionClear;
    colorAttachment.storeAction = MTLStoreActionStore;
    colorAttachment.clearColor = MTLClearColorMake(0, 0, 0, 0);
    //colorAttachment.clearColor = MTLClearColorMake(1, 0, 0, 0);
    
    auto depthAttachment = renderPassDescriptor.depthAttachment;
    
    depthAttachment.texture = cp_drawable_get_depth_texture(_drawable, 0);
    depthAttachment.loadAction = MTLLoadActionClear;
    depthAttachment.storeAction = MTLStoreActionStore;
    depthAttachment.clearDepth = 0.0;
    
    return renderPassDescriptor;
}

void
CompositorServicesRendererDelegate::presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer)
{
    if (_drawable != nullptr)
    {
        cp_drawable_encode_present(_drawable, commandBuffer);
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
