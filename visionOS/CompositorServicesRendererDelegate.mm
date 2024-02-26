//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "CompositorServicesRendererDelegate.h"
#import <Spatial/Spatial.h>

CompositorServicesRendererDelegate::CompositorServicesRendererDelegate(cp_layer_renderer_t layerRenderer)
: _layerRenderer(layerRenderer)
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
    _arSession = ar_session_create();
    
    auto config = ar_world_tracking_configuration_create();
    _worldTracking = ar_world_tracking_provider_create(config);
    
    _renderer = renderer;
    
    _cameraInfos.resize(cameraInfoCount());
    
    _configuration = std::make_shared<RenderTargetConfiguration>();
    _configuration->colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    _configuration->depthPixelFormat = MTLPixelFormatDepth32Float;
    
   _deviceAnchor = ar_device_anchor_create();
    
    return true;
}

void
CompositorServicesRendererDelegate::startRenderLoop()
{
    auto providers = ar_data_providers_create();
    ar_data_providers_add_data_provider(providers, _worldTracking);
    
    ar_session_run(_arSession, providers);
    
    _renderThread = std::thread { [this]()
    {
        while (!_shouldStopRendering)
        {
            _renderer->render();
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

void
CompositorServicesRendererDelegate::startSubmission()
{
    cp_frame_start_submission(_frame);
    
    cp_frame_timing_t timing = cp_frame_predict_timing(_frame);
    const cp_time_t t = cp_frame_timing_get_presentation_time(timing);
    const CFTimeInterval timeStamp = cp_time_to_cf_time_interval(t);
    
    const auto status = ar_world_tracking_provider_query_device_anchor_at_timestamp(_worldTracking, timeStamp, _deviceAnchor);
    ASSERT(status == ar_device_anchor_query_status_success);
    
    _drawable = cp_frame_query_drawable(_frame);
    if (_drawable == nullptr)
    {
        return;
    }
    
    const size_t nbViews = cp_drawable_get_view_count(_drawable);
    ASSERT(nbViews == _cameraInfos.size());
    
    cp_drawable_set_device_anchor(_drawable, _deviceAnchor);
    
    const float4x4 deviceAnchorTransform =  ar_anchor_get_origin_from_anchor_transform(_deviceAnchor);
    
    // hack
    const auto& cameras = _renderer->cameraRig()->cameras();
    
    const size_t n = _cameraInfos.size();
    for (size_t i=0; i < n; ++i)
    {
        const cp_view_t view = cp_drawable_get_view(_drawable, i);
        
        const float4x4 cameraTransformInAnchorSpace = cp_view_get_transform(view);
        const float4x4 worldCameraTransform = deviceAnchorTransform * cameraTransformInAnchorSpace;
        
        //const auto viewMatrix = inverse(worldCameraTransform);
        cameras[i]->setWorldTransform(worldCameraTransform);
        
        const float4 tangents = cp_view_get_tangents(view);
        
        const auto depthRange = cp_drawable_get_depth_range(_drawable);
        
        const auto projection = SPProjectiveTransform3DMakeFromTangents(tangents[0],
                                                                        tangents[1],
                                                                        tangents[2],
                                                                        tangents[3],
                                                                        depthRange.y,
                                                                        depthRange.x,
                                                                        true);
        
        auto& cameraInfo = _cameraInfos[i];
        cameraInfo.setProjectionMatrix(convert(projection.matrix));
        
        const auto textureMap = cp_view_get_view_texture_map(view);
        const MTLViewport viewport =  cp_view_texture_map_get_viewport(textureMap);
        
        const float2 viewportSize { float(viewport.width), float(viewport.height) };
        
        cameraInfo.setViewportSize(viewportSize);
        cameraInfo.setViewportSizeInPoints(viewportSize);
    }
        
}

void
CompositorServicesRendererDelegate::endSubmission()
{
    cp_frame_end_submission(_frame);
    
    _frame = nil;
    _drawable = nil;
}

size_t
CompositorServicesRendererDelegate::cameraInfoCount() const
{
#if TARGET_OS_SIMULATOR
    return 1;
#else
    return 2;
#endif
}

CameraInfo
CompositorServicesRendererDelegate::cameraInfo(size_t index, const Camera::Ptr& camera) const
{
    return _cameraInfos[index];
}

MTLRenderPassDescriptor* _Nullable
CompositorServicesRendererDelegate::currentRenderPassDescriptor() const
{
    auto renderPassDescriptor = [MTLRenderPassDescriptor new];
    
    auto colorAttachment = renderPassDescriptor.colorAttachments[0];
    
    colorAttachment.texture = cp_drawable_get_color_texture(_drawable, 0);;
    
    colorAttachment.loadAction = MTLLoadActionClear;
    colorAttachment.storeAction = MTLStoreActionStore;
    colorAttachment.clearColor = MTLClearColorMake(0, 0, 0, 0);
    
    auto depthAttachment = renderPassDescriptor.depthAttachment;
    
    depthAttachment.texture = cp_drawable_get_depth_texture(_drawable, 0);
    depthAttachment.loadAction = MTLLoadActionClear;
    depthAttachment.storeAction = MTLStoreActionStore;
    depthAttachment.clearDepth = 0.0;
    
    /*
    renderPassDescriptor.rasterizationRateMap = drawable.rasterizationRateMaps.first
    if layerRenderer.configuration.layout == .layered {
        renderPassDescriptor.renderTargetArrayLength = drawable.views.count
    }*/
    
    return renderPassDescriptor;
}

void
CompositorServicesRendererDelegate::presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer)
{
    cp_drawable_encode_present(_drawable, commandBuffer);
}

void
CompositorServicesRendererDelegate::invalidate()
{
    
}

void
CompositorServicesRendererDelegate::pause()
{
    
}
