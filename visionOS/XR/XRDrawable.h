//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"
#import <ARKit/ARKit.h>
#import <CompositorServices/CompositorServices.h>

#include <memory>
#include <functional>

@class XRDrawableImpl;

class XRDrawable final
{
public:
    using Ptr = std::unique_ptr<XRDrawable>;
    
    XRDrawable(XRDrawableImpl*_Nonnull);
    ~XRDrawable();
    
    XRDrawableImpl* _Nullable impl() const
    {
        return _impl;
    }
    
    size_t viewCount() const;
    float4x4 localEyeTransform(size_t index) const;
    float4 tangents(size_t index) const;
    MTLViewport viewport(size_t index) const;
    id<MTLTexture> _Nullable colorTexture(size_t index) const;
    id<MTLTexture> _Nullable depthTexture(size_t index) const;
    id<MTLRasterizationRateMap> _Nullable rasterizationRateMaps(size_t index) const;
    
    float2 depthRange() const;
    
    void present(id<MTLCommandBuffer> _Nonnull cmdBuffer);
    
private:
    XRDrawableImpl* const _Nonnull _impl;
};
