//
//  XRDragInteraction.hpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#pragma once

#include "XR/XRHandAnchor.h"
#include "Object3D.h"

#include <memory>

class XRInteraction
{
public:
    using Ptr = std::shared_ptr<XRInteraction>;
    
    virtual ~XRInteraction() = default;
    
    enum class State
    {
        inactive,
        possible,
        active
    };
    
    State state() const { return _state; }
    
    virtual void update(const XRHandAnchors&) = 0;
    virtual void commit() = 0;
    
protected:
    void _setState(State);
    
private:
    State _state = State::inactive;
};

class XRHandAnchorsWithDistance : public XRHandAnchors
{
public:
    using _inherited = XRHandAnchors;
    XRHandAnchorsWithDistance(const XRHandAnchors&);
    
    struct Distance final
    {
        std::optional<float3> position;
        
        Object3D::Ptr object;
        float distance = 1e10f;
    };
    
    std::array<Distance, kMaxChirality> distances;
    
    const Distance& distance(Chirality) const;
    
    void updateDistance(const Object3D::Ptr& o);
    
    std::optional<Chirality> closestAnchorChirality() const;
};

void findClosestObject(const Object3D::Ptr& object, XRHandAnchorsWithDistance& anchors);

std::optional<float3> worldTipPosition(const XRHandAnchor::Ptr& anchor);
float3 worldTipPosition(const XRHandAnchor& anchor);
