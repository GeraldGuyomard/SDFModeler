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
XRDualPinchInteraction::update(const XRHandAnchors& anchors)
{
    size_t nbTrackedHands = 0;
    
    for (const auto& anchor : anchors.anchors)
    {
        if (anchor != nullptr)
        {
            ++nbTrackedHands;
            
            if (!anchor->isPinching())
            {
                _setState(State::inactive);
                return;
            }
        }
    }
    
    if (nbTrackedHands != 2)
    {
        return;
    }
    
    const auto pos0 = worldTipPosition(*anchors.anchors[0]);
    const auto pos1 = worldTipPosition(*anchors.anchors[1]);
    
    if (state() != State::active)
    {
        const auto d = length(pos0 - pos1);
        
        XRHandAnchorsWithDistance anchorsWithDist { anchors };
        
        findClosestObject(_world->rootObject(), anchorsWithDist);
        const auto chiralityOpt = anchorsWithDist.closestAnchorChirality();
        
        const auto object = anchorsWithDist.distances[size_t(chiralityOpt.value())].object;
        
        _activeState = std::make_unique<ActiveState>(d, object);
        _setState(State::active);
        
        object->world()->commandHistory().enable(false);
    }
    
    
    const auto& left = anchors.anchor(Chirality::left);
    const auto& right = anchors.anchor(Chirality::right);
    
    const auto newPos0 = translation(left->worldTransform());
    const auto newPos1 = translation(right->worldTransform());
    
    const float newDist = length(newPos1 - newPos0);
    if (newDist < 1e-3f)
    {
        return;
    }
    
    auto& activeState = *_activeState;
    
    const float scale = 1.f + (newDist - activeState.initialDistance) / newDist;
    
    const auto pos = translation(activeState.entry.transform);
    const auto moveToPivot = matrix4x4_translation(-pos);
    const auto moveBackToPos = matrix4x4_translation(pos);
    const auto scaleM = matrix4x4_scale(scale);
    
    const auto transform = moveBackToPos * scaleM * moveToPivot * activeState.entry.transform;
    
    activeState.entry.object->setWorldTransform(transform);
}

void
XRDualPinchInteraction::commit()
{
    if (_activeState != nullptr)
    {
        auto command = std::make_shared<TransformObjectCommand>(std::vector<TransformObjectCommand::Entry>{ _activeState->entry });
        
        _world->commandHistory().run(command);
        _world->commandHistory().enable(true);
        
        _activeState.reset();
    }
}
