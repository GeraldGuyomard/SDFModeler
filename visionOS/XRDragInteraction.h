//
//  XRDragInteraction.hpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#pragma once

#include "XR/XRHandAnchor.h"
#include "Object3D.h"
#include "TransformObjectCommand.h"
#include "XRInteraction.h"

class XRDragInteraction final : public XRInteraction
{
public:
    using Ptr = std::shared_ptr<XRDragInteraction>;
    
    XRDragInteraction(const WorldPtr& world);
    
    State update(const XRHandAnchors&) override;
    
    struct StatePayload final
    {
        const Chirality chirality;
        float3 initialPosInWorld;
        
        TransformObjectCommand::Entry entry;
        
        StatePayload(Chirality c, float3 pos, const Object3D::Ptr& object )
        : chirality(c), initialPosInWorld(pos), entry { object }
        {}
    };
    
    const StatePayload* statePayload() const
    {
        return _statePayload.get();
    }
    
    static constexpr float kMinDistanceToAnyObjectForActivation = 0.02f;
    
protected:
    void _onStateChanged(State oldState, State newState) override;
    
private:
    
    State _updateWhenInactive(const XRHandAnchors&);
    State _updateWhenPossible(const XRHandAnchors&);
    State _updateWhenActive(const XRHandAnchors&);
    
    WorldPtr _world;
    
    std::unique_ptr<StatePayload> _statePayload;
};

