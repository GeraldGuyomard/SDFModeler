//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRDualPinchInteraction.h"

XRDualPinchInteraction::XRDualPinchInteraction(const WorldPtr& world)
: _world(world)
{
}

void
XRDualPinchInteraction::_onStateChanged(State oldState, State newState)
{
    switch(newState)
    {
        case State::inactive:
        {
            ASSERT(_activePayload != nullptr);
            
            auto command = std::make_shared<TransformObjectCommand>(std::vector<TransformObjectCommand::Entry>{ _activePayload->entry });
            
            _world->commandHistory().run(command);
            _world->commandHistory().enable(true);
            
            _activePayload.reset();
            break;
        }
            
        case State::active:
        {
            ASSERT(_activePayload != nullptr);
            _world->commandHistory().enable(false);
            break;
        }
            
        default:
        {
            ASSERT(false);
            break;
        }
    }
}

constexpr float kMinDistance = 0.05f;

XRInteraction::State
XRDualPinchInteraction::_updateWhenInactive(const XRHandAnchors& anchors)
{
    ASSERT(_activePayload == nullptr);
    
    size_t nbPinchingHands = 0;
    
    for (const auto& anchor : anchors.anchors)
    {
        if ((anchor != nullptr) && anchor->isPinching())
        {
            ++nbPinchingHands;
        }
    }
    
    if (nbPinchingHands != 2)
    {
        return State::inactive;
    }
    
    XRHandAnchorsWithDistance anchorsWithDist { anchors };
    
    findClosestObject(_world->rootObject(), anchorsWithDist);
    const auto chiralityOpt = anchorsWithDist.closestAnchorChirality();
    if (!chiralityOpt.has_value())
    {
        return State::inactive;
    }
    
    const auto object = anchorsWithDist.distances[size_t(chiralityOpt.value())].object;
    
    const auto posLeft = worldTipPosition(*anchors.anchor(Chirality::left));
    const auto posRight = worldTipPosition(*anchors.anchor(Chirality::right));
    
    const auto d = length(posLeft - posRight);
    if (d < kMinDistance)
    {
        return State::inactive;
    }
    
    _activePayload = std::make_unique<ActivePayload>(d, object);
    
    return State::active;
}

XRInteraction::State
XRDualPinchInteraction::_updateWhenActive(const XRHandAnchors& anchors)
{
    ASSERT(_activePayload != nullptr);
    
    const auto& left = anchors.anchor(Chirality::left);
    const auto& right = anchors.anchor(Chirality::right);
    
    if ((left == nullptr) || (right == nullptr))
    {
        // lost tracking
        return State::active;
    }
    
    if (!left->isPinching() && !right->isPinching())
    {
        return State::inactive;
    }
    
    const auto posLeft = worldTipPosition(*anchors.anchor(Chirality::left));
    const auto posRight = worldTipPosition(*anchors.anchor(Chirality::right));
    
    const auto d = length(posLeft - posRight);
    
    if (d < kMinDistance)
    {
        return State::active;
    }
    
    auto& payload = *_activePayload;
    
    const float scale = 1.f + (d - payload.initialDistance) / d;
    NSLog(@"scale=%5.3f newDist=%5.3f initialDistance=%5.3f", scale, d, payload.initialDistance);
    
    const auto pos = translation(payload.entry.transform);
    const auto moveToPivot = matrix4x4_translation(-pos);
    const auto moveBackToPos = matrix4x4_translation(pos);
    const auto scaleM = matrix4x4_scale(scale);
    
    const auto transform = moveBackToPos * scaleM * moveToPivot * payload.entry.transform;
    
    payload.entry.object->setWorldTransform(transform);
    
    return State::active;
}

XRInteraction::State
XRDualPinchInteraction::update(const XRHandAnchors& anchors)
{
    const auto state = this->state();
    switch (state)
    {
        case State::inactive: return _updateWhenInactive(anchors);
        case State::active: return _updateWhenActive(anchors);
        default:
        {
            ASSERT(false);
            return state;
        }
    }
}

