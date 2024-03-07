//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRInteraction.h"



std::optional<float3> worldTipPosition(const XRHandAnchor::Ptr& anchor)
{
    if (anchor == nullptr)
    {
        return std::nullopt;
    }
    
    const auto tipTransform = anchor->jointTransformInWorldSpace(JointID::indexFingerTip);
    
    return translation(tipTransform);
}

float3 worldTipPosition(const XRHandAnchor& anchor)
{
    const auto tipTransform = anchor.jointTransformInWorldSpace(JointID::indexFingerTip);
    return translation(tipTransform);
}

XRHandAnchorsWithDistance::XRHandAnchorsWithDistance(const XRHandAnchors& anchors)
: _inherited(anchors)
{
    for (size_t i=0; i < 2; ++i)
    {
        distances[i].position = worldTipPosition(anchors.anchors[i]);
    }
}
    
const XRHandAnchorsWithDistance::Distance&
XRHandAnchorsWithDistance::distance(Chirality c) const
{
    return distances[size_t(c)];
}

void
XRHandAnchorsWithDistance::updateDistance(const Object3D::Ptr& o)
{
    for (auto& dist : distances)
    {
        if (!dist.position.has_value())
        {
            continue;
        }
        
        const float d = o->computeDistance(dist.position.value());
        if (d < dist.distance)
        {
            dist.distance = d;
            dist.object = o;
        }
    }
}

std::optional<Chirality>
XRHandAnchorsWithDistance::closestAnchorChirality() const
{
    float m = std::numeric_limits<float>::max();
    std::optional<Chirality> c;
    for (size_t i=0; i < 2; ++i)
    {
        if (distances[i].distance < m)
        {
            m = distances[i].distance;
            c = anchors[i]->chirality();
        }
    }
    
    return c;
}

void
XRInteraction::_setState(State s)
{
    _state = s;
}
