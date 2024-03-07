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
XRDragInteraction::update(const XRHandAnchors& anchors)
{
    if (_activeState != nullptr)
    {
        ASSERT(state() == State::active);
        constexpr float kPinchingReleaseDist = XRHandAnchor::kDefaultFingerDistance * 2.f;
        
        const auto& anchor = anchors.anchor(_activeState->chirality);
        
        if ((anchor != nullptr) && !anchor->isPinching(kPinchingReleaseDist))
        {
            _setState(State::inactive);
        }
        else
        {
            const auto& otherAnchor = anchors.otherAnchor(_activeState->chirality);
            if ((otherAnchor != nullptr) && otherAnchor->isPinching())
            {
                _setState(State::inactive);
            }
        }
    }
    else
    {
        const XRHandAnchor* activeAnchor = nullptr;
        for (const auto& anchor : anchors.anchors)
        {
            if ((anchor != nullptr) && anchor->isPinching())
            {
                if (activeAnchor != nullptr)
                {
                    // more than one -> discard
                    _setState(State::inactive);
                }
                else
                {
                    activeAnchor = anchor.get();
                }
                
                break;
            }
        }
        
        XRHandAnchorsWithDistance anchorsWithDist { anchors };
        
        findClosestObject(_world->rootObject(), anchorsWithDist);
        
        if (activeAnchor != nullptr)
        {
            const auto& distances = anchorsWithDist.distances[size_t(activeAnchor->chirality())];
            if (distances.position.has_value()) {
                _activeState = std::make_unique<ActiveState>(
                    activeAnchor->chirality(),
                    distances.position.value(),
                    distances.object
                );
                
                _setState(State::possible);
                
                const auto minChirality = anchorsWithDist.closestAnchorChirality();
                if (minChirality.has_value())
                {
                    auto object = anchorsWithDist.distances[size_t(minChirality.value())].object;
                    object->world()->commandHistory().enable(false);
                }
            }
        }
    }
    
    const auto& activeAnchor = anchors.anchor(_activeState->chirality);
    
    const auto newPos = translation(activeAnchor->jointTransformInWorldSpace(JointID::indexFingerTip));
    
    if (state() == State::inactive)
    {
        _setState(State::possible);
    }
    else if (state() == State::possible)
    {
        const auto delta = newPos - _activeState->initialPosInWorld;
        const float d = length(delta);
        if (d >= 0.05f)
        {
            _activeState->initialPosInWorld = newPos;
            _setState(State::active);
        }
    }
    else
    {
        const auto delta = newPos - _activeState->initialPosInWorld;
        
        auto transform = _activeState->entry.transform;
        const auto newWorldPos = translation(transform) + delta;
        setTranslation(transform, newWorldPos);
        
        _activeState->entry.object->setWorldTransform(transform);
        
        _setState(State::active);
    }
}

void
XRDragInteraction::commit()
{
    if (_activeState != nullptr)
    {
        auto command = std::make_shared<TransformObjectCommand>(std::vector<TransformObjectCommand::Entry>{ _activeState->entry });
        
        _world->commandHistory().run(command);
        _world->commandHistory().enable(true);
        
        _activeState.reset();
    }
}
