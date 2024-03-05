//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"

#include <memory>

@class XRHandTrackingImpl, XRHandAnchorImpl;

class XRHandAnchor final
{
public:
    using Ptr = std::unique_ptr<XRHandAnchor>;
    
    XRHandAnchor(XRHandAnchorImpl* _Nonnull impl);
    ~XRHandAnchor();
    
    bool isTracked() const;
    float4x4 worldTransform() const;
    
private:
    XRHandAnchorImpl* const _Nonnull _impl;
};

class XRHandTracking final
{
public:
    using Ptr = std::unique_ptr<XRHandTracking>;
    
    XRHandTracking(XRHandTrackingImpl*_Nonnull);
    ~XRHandTracking();
    
    const XRHandAnchor* _Nullable leftHand() const { return _leftHand.get(); }
    const XRHandAnchor* _Nullable rightHand() const { return _rightHand.get(); }
    
private:
    XRHandAnchor::Ptr _leftHand;
    XRHandAnchor::Ptr _rightHand;
};
