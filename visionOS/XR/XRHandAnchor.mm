//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRHandAnchor.h"
#import "SDFModeler_visionOS-Swift.h"


XRHandTracking::XRHandTracking(XRHandTrackingImpl* impl)
{
    auto leftHandImpl = [impl leftHand];
    if (leftHandImpl != nil)
    {
        _leftHand = std::make_unique<XRHandAnchor>(leftHandImpl);
    }
    
    auto rightHandImpl = [impl rightHand];
    if (rightHandImpl != nil)
    {
        _rightHand = std::make_unique<XRHandAnchor>(rightHandImpl);
    }
}

XRHandTracking::~XRHandTracking() = default;

XRHandAnchor::XRHandAnchor(XRHandAnchorImpl* impl)
: _impl(impl)
{}

XRHandAnchor::~XRHandAnchor() = default;

bool
XRHandAnchor::isTracked() const
{
    return _impl.isTracked;
}

float4x4
XRHandAnchor::worldTransform() const
{
    return [_impl worldTransform];
}

