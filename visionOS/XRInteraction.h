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
#include <string>
#include <functional>

class XRInteraction : public std::enable_shared_from_this<XRInteraction>
{
public:
    using Ptr = std::shared_ptr<XRInteraction>;
    
    virtual ~XRInteraction() = default;
    
    const std::string& name() const { return _name; }
    void setName(const std::string& n) { _name = n; }
    
    enum class State
    {
        inactive,
        possible,
        active
    };
    
    State state() const { return _state; }
    
    virtual State update(const XRHandAnchors&) = 0;
    
    using StateChangedCallback = std::function<void(const XRInteraction::Ptr& interaction, XRInteraction::State oldState, XRInteraction::State newState)>;
    void setStateChangedCallback(const StateChangedCallback&);
                                                                            
protected:
    virtual void _onStateChanged(State oldState, State newState) {}
    
    void _setState(State);
    
private:
    
    friend class XRInteractionManager;
    
    State _state = State::inactive;
    StateChangedCallback _stateChangedCallback;
    
    std::string _name;
};

class XRInteractionManager final
{
public:
    XRInteractionManager() = default;
    
    void process(const XRHandAnchors&);
    
    void add(const XRInteraction::Ptr&);
    
    const std::vector<XRInteraction::Ptr>& interactions() const
    {
        return _interactions;
    }
    
private:
    std::vector<XRInteraction::Ptr> _interactions;
    XRInteraction::Ptr _activeInteraction;
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
