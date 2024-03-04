//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRService.h"
#import "SDFModeler_visionOS-Swift.h"

XRFrame::XRFrame(XRFrameImpl* impl)
: _impl(impl)
{}

XRFrame::~XRFrame() = default;

void
XRFrame::startUpdate()
{
    [_impl startUpdate];
}

void
XRFrame::endUpdate()
{
    [_impl endUpdate];
}

bool
XRFrame::waitUntilOptimalTime()
{
    return [_impl waitUntilOptimalTime];
}

void
XRFrame::startSubmission()
{
    [_impl startSubmission];
}

void
XRFrame::endSubmission()
{
    [_impl endSubmission];
}

XRDrawable::XRDrawable(XRDrawableImpl* impl)
: _impl(impl)
{}

XRDrawable::~XRDrawable() = default;

size_t
XRDrawable::viewCount() const
{
    return [_impl viewCount];
}

float4x4
XRDrawable::localEyeTransform(size_t index) const
{
    return [_impl localEyeTransform:index];
}

float4
XRDrawable::tangents(size_t index) const
{
    return [_impl tangents:index];
}

MTLViewport
XRDrawable::viewport(size_t index) const
{
    return [_impl viewport:index];
}

float2
XRDrawable::depthRange() const
{
    return [_impl depthRange];
}

id<MTLTexture>
XRDrawable::colorTexture(size_t index) const
{
    return [_impl colorTexture:index];
}

id<MTLTexture>
XRDrawable::depthTexture(size_t index) const
{
    return [_impl depthTexture:index];
}

id<MTLRasterizationRateMap> _Nullable
XRDrawable::rasterizationRateMaps(size_t index) const
{
    return [_impl rasterizationRateMap:index];
}

void
XRDrawable::present(id<MTLCommandBuffer> cmdBuffer)
{
    [_impl present:cmdBuffer];
}

XRService::XRService()
{}

XRService::~XRService()
{
}

XRService::Ptr
XRService::make()
{
    Ptr service { new XRService };
    if (!service->_init())
    {
        return nullptr;
    }
    
    return service;
}

bool
XRService::_init()
{
    _impl = [XRServiceImpl new];
    
    return true;
}

void
XRService::start(const Completion& completion)
{
    const auto completionCopy = completion;
    
    [_impl startWithCompletion:^{
        completionCopy();
    }];
}

XRFrame
XRService::queryNextFrame(cp_layer_renderer_t layerRenderer)
{
    auto impl = [_impl queryNextFrameWithLayerRenderer:layerRenderer];
    if (impl == nil)
    {
        return {};
    }
    
    return {impl};
}

XRDrawable
XRService::queryDrawable(const XRFrame& frame)
{
    return { [_impl queryDrawableWithFrame:frame.impl()] };
}

float4x4
XRService::worldHeadTransform(const XRDrawable& d) const
{
    return [_impl worldHeadTransform:d.impl()];
}
