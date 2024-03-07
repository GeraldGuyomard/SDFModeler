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
    const float4x4 m = [_impl worldTransform];
    
    /*
    const auto r = right(m);
    const auto u = up(m);
    const auto f = forward(m);
    
    NSLog(@"chirality:%d Hand's rt %5.2f, %5.2f, %5.2f", int(chirality()), r.x, r.y, r.z );
    NSLog(@"                    up %5.2f, %5.2f, %5.2f", u.x, u.y, u.z );
    NSLog(@"                    fw %5.2f, %5.2f, %5.2f", f.x, f.y, f.z );*/
    
    return m;
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
        case JointID::wrist: idImpl = JointIDImplWrist; break;
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

const XRHandAnchor::Ptr&
XRHandAnchors::anchor(Chirality c) const
{
    return anchors[size_t(c)];
}
XRHandAnchor::Ptr&
XRHandAnchors::anchor(Chirality c)
{
    return anchors[size_t(c)];
}

const XRHandAnchor::Ptr&
XRHandAnchors::otherAnchor(Chirality c) const
{
    size_t i = 1 - size_t(c);
    return anchors[i];
}

XRHandAnchor::Ptr&
XRHandAnchors::otherAnchor(Chirality c)
{
    size_t i = 1 - size_t(c);
    return anchors[i];
}

bool
XRHandAnchors::none() const
{
    return (anchors[0] == nullptr) && (anchors[1] == nullptr);
}
