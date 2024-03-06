//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRDragInteraction.h"

XRDragInteraction::XRDragInteraction(Chirality handChirality,
                  float3 initialDraggingPosInWorld,
                    JointID jointID,
                  const Object3D::Ptr& object)
: _handChirality(handChirality),
_jointID(jointID),
_initialDraggingPosInWorld(initialDraggingPosInWorld),
_entry(object)
{
    object->world()->commandHistory().enable(false);
}


bool
XRDragInteraction::update(const XRHandAnchor* left, const XRHandAnchor* right)
{
    auto hand = (_handChirality == Chirality::left) ? left : right;
    if (hand == nullptr)
    {
        // hand lost, keep going
        return true;
    }
    
    constexpr float kPinchingReleaseDist = XRHandAnchor::kDefaultFingerDistance * 2.f;
    if (!hand->isPinching(kPinchingReleaseDist))
    {
        return false;
    }
    
    
    const auto newDragPos = translation(hand->jointTransformInWorldSpace(_jointID));
    const auto delta = newDragPos - _initialDraggingPosInWorld;
    
    auto transform = _entry.transform;
    const auto newWorldPos = translation(transform) + delta;
    setTranslation(transform, newWorldPos);
    
    _entry.object->setWorldTransform(transform);
    
    return true;
}

void
XRDragInteraction::commit()
{
    auto command = std::make_shared<TransformObjectCommand>(std::vector<TransformObjectCommand::Entry>{ _entry });
    
    auto world = _entry.object->world();
    world->commandHistory().run(command);
    world->commandHistory().enable(true);
}
