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
    XRHandAnchorImpl* _Nonnull _impl;
};

class XRHandTracking final
{
public:
    using Ptr = std::unique_ptr<XRHandTracking>;
    
    XRHandTracking(XRHandTrackingImpl*_Nonnull);
    ~XRHandTracking();
    
    const XRHandAnchor* leftHand() const { return _leftHand.get(); }
    const XRHandAnchor* rightHand() const { return _rightHand.get(); }
    
private:
    XRHandAnchor::Ptr _leftHand;
    XRHandAnchor::Ptr _rightHand;
};
