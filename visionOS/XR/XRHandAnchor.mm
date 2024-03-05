//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRHandAnchor.h"
#import <ARKit/ARKit.h>
#import <Metal/Metal.h>

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

Chirality
XRHandAnchor::chirality() const
{
    switch (_impl.chirality)
    {
        case ChiralityImplLeft: return Chirality::left;
        case ChiralityImplRight: return Chirality::right;
    }
}

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

float4x4
XRHandAnchor::jointTransform(JointID id) const
{
    JointIDImpl idImpl;
    
    switch (id)
    {
        case JointID::thumbTip : idImpl = JointIDImplThumbTip; break;
        case JointID::indexFingerTip: idImpl = JointIDImplIndexFingerTip; break;
        case JointID::middleFingerTip: idImpl = JointIDImplIndexFingerTip; break;
        case JointID::ringFingerTip: idImpl = JointIDImplRingFingerTip; break;
        case JointID::littleFingerTip: idImpl = JointIDImplLittleFingerTip; break;
    }
    
    return [_impl jointTransform:idImpl];
}

