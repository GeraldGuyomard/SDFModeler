//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRInteraction.h"


void
XRInteractionManager::process(const XRHandAnchors& anchors)
{
    XRInteraction::Ptr previousActiveInteraction;
    if (_activeInteraction != nullptr)
    {
        ASSERT(_activeInteraction->state() == XRInteraction::State::active);
        const auto requiredState = _activeInteraction->update(anchors);
        if (requiredState == XRInteraction::State::active)
        {
            // exclusive, one active interaction at a time
            return;
        }

        _activeInteraction->_setState(requiredState);
        previousActiveInteraction = _activeInteraction;
        _activeInteraction.reset();
    }
    
    for (const auto& interaction : _interactions)
    {
        if (interaction == previousActiveInteraction)
        {
            continue;
        }
        
        const auto requiredState = interaction->update(anchors);
        interaction->_setState(requiredState);
        
        if (requiredState == XRInteraction::State::active)
        {
            _activeInteraction = interaction;
            break;
        }
    }
}

void
XRInteractionManager::add(const XRInteraction::Ptr& interaction)
{
    _interactions.push_back(interaction);
    interaction->_setState(XRInteraction::State::inactive);
}

void
XRInteraction::_setState(State state)
{
    if (_state != state)
    {
        const auto oldState = _state;
        _state = state;
        _onStateChanged(oldState, _state);
        
        if (_stateChangedCallback != nullptr)
        {
            _stateChangedCallback(shared_from_this(), oldState, state);
        }
    }
}

void
XRInteraction::setStateChangedCallback(const StateChangedCallback& cb)
{
    _stateChangedCallback = cb;
}

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
        auto anchor = anchors[i];
        if (anchor != nullptr)
        {
            if (distances[i].distance < m)
            {
                m = distances[i].distance;
                c = anchor->chirality();
            }
        }
    }
    
    return c;
}
