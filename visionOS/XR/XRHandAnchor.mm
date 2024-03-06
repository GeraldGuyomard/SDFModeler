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
XRHandAnchor::jointTransformInHandSpace(JointID id) const
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
    
    return [_impl jointTransformInHandSpace:idImpl];
}

float4x4
XRHandAnchor::jointTransformInWorldSpace(JointID id) const
{
    return worldTransform() * jointTransformInHandSpace(id);
}

bool
XRHandAnchor::isPinching(float minDistance) const
{
    const auto indexTipPos = translation(jointTransformInHandSpace(JointID::indexFingerTip));
    const auto thumbTipPos = translation(jointTransformInHandSpace(JointID::thumbTip));
    
    const float d = length(indexTipPos - thumbTipPos);
    
    return d <= minDistance;
}
