//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRDragInteraction.h"

void findClosestObject(const Object3D::Ptr& object, XRHandAnchorsWithDistance& anchors)
{
    anchors.updateDistance(object);
    
    for (const auto& child : object->children())
    {
        findClosestObject(child, anchors);
    }
}

XRDragInteraction::XRDragInteraction(const WorldPtr& world)
: _world(world)
{
}


void
XRDragInteraction::_onStateChanged(State oldState, State newState)
{
    switch (newState)
    {
        case State::inactive:
        {
            ASSERT(_statePayload != nullptr);
            
            if (oldState == State::active)
            {
                auto command = std::make_shared<TransformObjectCommand>(std::vector<TransformObjectCommand::Entry>{ _statePayload->entry });
                _world->commandHistory().run(command);
                _world->commandHistory().enable(true);
            }
            
            _statePayload.reset();
            break;
        }
            
        case State::possible: break;
            
        case State::active:
        {
            auto object = _statePayload->entry.object;
            object->world()->commandHistory().enable(false);
            break;
        }
    }
}

XRInteraction::State
XRDragInteraction::_updateWhenInactive(const XRHandAnchors& anchors)
{
    ASSERT(_statePayload == nullptr);
    
    const XRHandAnchor* activeAnchor = nullptr;
    for (const auto& anchor : anchors.anchors)
    {
        if ((anchor != nullptr) && anchor->isPinching())
        {
            if (activeAnchor != nullptr)
            {
                // more than one -> discard
                return State::inactive;
            }
            else
            {
                activeAnchor = anchor.get();
            }
            
            break;
        }
    }
    
    if (activeAnchor == nullptr)
    {
        return State::inactive;
    }
    
    XRHandAnchorsWithDistance anchorsWithDist { anchors };
    
    findClosestObject(_world->rootObject(), anchorsWithDist);

    const auto& distances = anchorsWithDist.distances[size_t(activeAnchor->chirality())];
    if (!distances.position.has_value()) {
        return State::inactive;
    }
    
    if (distances.distance > kMinDistanceForActivation)
    {
        return State::inactive;
    }
    
    _statePayload = std::make_unique<StatePayload>(
        activeAnchor->chirality(),
        distances.position.value(),
        distances.object
    );
    
    return State::possible;
}

XRInteraction::State
XRDragInteraction::_updateWhenPossible(const XRHandAnchors& anchors)
{
    ASSERT(_statePayload != nullptr);
    
    const auto& activeAnchor = anchors.anchor(_statePayload->chirality);
    if (activeAnchor == nullptr)
    {
        // tracking lost
        return State::possible;
    }
    
    const auto newPos = translation(activeAnchor->jointTransformInWorldSpace(JointID::indexFingerTip));
    
    const auto delta = newPos - _statePayload->initialPosInWorld;
    const float d = length(delta);
    if (d >= 0.05f)
    {
        _statePayload->initialPosInWorld = newPos;
        return State::active;
    }
    
    return State::possible;
}

XRInteraction::State
XRDragInteraction::_updateWhenActive(const XRHandAnchors& anchors)
{
    ASSERT(_statePayload != nullptr);
    
    const auto& activeAnchor = anchors.anchor(_statePayload->chirality);
    if (activeAnchor == nullptr)
    {
        // lost track
        return State::active;
    }
    
    constexpr float kPinchingReleaseDist = XRHandAnchor::kDefaultFingerDistance * 2.f;
    if (!activeAnchor->isPinching(kPinchingReleaseDist))
    {
        return State::inactive;
    }
    
    const auto anchorPos = translation(activeAnchor->jointTransformInWorldSpace(JointID::indexFingerTip));
    const auto delta = anchorPos - _statePayload->initialPosInWorld;
    
    auto m = _statePayload->entry.transform;
    auto pos = translation(m);
    pos += delta;
    setTranslation(m, pos);
    
    _statePayload->entry.object->setWorldTransform(m);
    
    return State::active;
}

XRInteraction::State
XRDragInteraction::update(const XRHandAnchors& anchors)
{
    switch (state())
    {
        case State::inactive: return _updateWhenInactive(anchors);
        case State::possible: return _updateWhenPossible(anchors);
        case State::active: return _updateWhenActive(anchors);
    }
}
