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

float3 worldTipPosition(const XRHandAnchor& anchor)
{
    const auto tipTransform = anchor.jointTransformInWorldSpace(JointID::indexFingerTip);
    return translation(tipTransform);
}


XRHandAnchor::XRHandAnchor(XRHandAnchorImpl* impl)
: _impl(impl)
{}

XRHandAnchor::~XRHandAnchor() = default;

Chirality
XRHandAnchor::chirality() const
{
    if (!_chirality.has_value())
    {
        switch (_impl.chirality)
        {
            case ChiralityImplLeft: _chirality = Chirality::left; break;
            case ChiralityImplRight: _chirality = Chirality::right; break;
        }
    }
    
    return _chirality.value();
}

bool
XRHandAnchor::isTracked() const
{
    if (!_isTracked.has_value())
    {
        _isTracked = _impl.isTracked;
    }
    
    return _isTracked.value();
}

float4x4
XRHandAnchor::worldTransform() const
{
    if (!_worldTransform.has_value())
    {
        _worldTransform = [_impl worldTransform];
    }
    
    
    /*
    const auto r = right(m);
    const auto u = up(m);
    const auto f = forward(m);
    
    NSLog(@"chirality:%d Hand's rt %5.2f, %5.2f, %5.2f", int(chirality()), r.x, r.y, r.z );
    NSLog(@"                    up %5.2f, %5.2f, %5.2f", u.x, u.y, u.z );
    NSLog(@"                    fw %5.2f, %5.2f, %5.2f", f.x, f.y, f.z );*/
    
    return _worldTransform.value();
}

float4x4
XRHandAnchor::jointTransformInHandSpace(JointID id) const
{
    auto& transform = _jointTransformInHandSpace[size_t(id)];
    if (!transform.has_value())
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
            default:
            {
                ASSERT(false);
                transform = float4x4_identity();
                break;
            }
        }
        
        transform = [_impl jointTransformInHandSpace:idImpl];
    }
    
    return transform.value();
}

float4x4
XRHandAnchor::jointTransformInWorldSpace(JointID id) const
{
    return worldTransform() * jointTransformInHandSpace(id);
}

bool
XRHandAnchor::isPinching(float minDistance) const
{
    if (!_isPinching.has_value())
    {
        const auto indexTipPos = translation(jointTransformInHandSpace(JointID::indexFingerTip));
        const auto thumbTipPos = translation(jointTransformInHandSpace(JointID::thumbTip));
        
        const float d = length(indexTipPos - thumbTipPos);
        
        _isPinching = d <= minDistance;
    }
    
    return _isPinching.value();
}

XRHandAnchors::XRHandAnchors() = default;

XRHandAnchors::XRHandAnchors(const WorldPtr& world, const XRHandAnchor::Ptr& left, const XRHandAnchor::Ptr& right)
{
    _entries[0].handAnchor = left;
    _entries[1].handAnchor = right;
    
    for (auto& entry : _entries)
    {
        if (entry.handAnchor != nullptr)
        {
            entry.position = worldTipPosition(*entry.handAnchor);
        }
    }
    
    _updateDistances(world->rootObject());
}

void
XRHandAnchors::_updateDistances(const Object3D::Ptr& object)
{
    for (auto& entry : _entries)
    {
        if (entry.handAnchor != nullptr)
        {
            const float d = object->computeDistance(entry.position);
            if (d < entry.distance)
            {
                entry.distance = d;
                entry.object = object;
            }
        }
    }
    
    for (const auto& child : object->children())
    {
        _updateDistances(child);
    }
}

const XRHandAnchors::Entry&
XRHandAnchors::entry(Chirality c) const
{
    return _entries[size_t(c)];
}

const XRHandAnchor::Ptr&
XRHandAnchors::anchor(Chirality c) const
{
    return entry(c).handAnchor;
}


const XRHandAnchor::Ptr&
XRHandAnchors::otherAnchor(Chirality c) const
{
    Chirality i = Chirality(1 - size_t(c));
    return entry(i).handAnchor;
}

bool
XRHandAnchors::none() const
{
    return (_entries[0].handAnchor == nullptr) && (_entries[1].handAnchor == nullptr);
}

const XRHandAnchors::Entry*
XRHandAnchors::closestEntryToAnyHand() const
{
    float m = std::numeric_limits<float>::max();
    const Entry* e = nullptr;
    for (const auto& entry : _entries)
    {
        auto anchor = entry.handAnchor;
        if (anchor != nullptr)
        {
            if (entry.distance < m)
            {
                m = entry.distance;
                e = &entry;
            }
        }
    }
    
    return e;
}
