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
_object(object),
_initialObjectWorldTransform(object->worldTransform())
{}


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
    
    auto transform = _initialObjectWorldTransform;
    const auto newWorldPos = translation(transform) + delta;
    setTranslation(transform, newWorldPos);
    
    _object->setWorldTransform(transform);
    
    return true;
}
