//
//  XRDragInteraction.hpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#pragma once

#include "XR/XRHandAnchor.h"

#include "XRInteraction.h"
#include "Object3D.h"
#include "TransformObjectCommand.h"

#include <memory>
#include <array>

class XRDualPinchInteraction final : public XRInteraction
{
public:
    using Ptr = std::shared_ptr<XRDualPinchInteraction>;

    XRDualPinchInteraction(const WorldPtr& world);
    
    State update(const XRHandAnchors&) override;
    
    struct ActivePayload final
    {
        const float initialDistance;
        const TransformObjectCommand::Entry entry;
        
        ActivePayload(float dist, const Object3D::Ptr& object)
        : initialDistance(dist), entry { object }
        {}
    };
    
    const ActivePayload* activePayload() const { return _activePayload.get(); }
    
protected:
    void _onStateChanged(State oldState, State newState) override;
    
private:
    State _updateWhenInactive(const XRHandAnchors&);
    State _updateWhenActive(const XRHandAnchors&);
    
    WorldPtr _world;
    std::unique_ptr<ActivePayload> _activePayload;
    
};

