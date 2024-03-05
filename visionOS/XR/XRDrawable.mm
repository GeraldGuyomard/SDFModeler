//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRDrawable.h"
#import "SDFModeler_visionOS-Swift.h"

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
