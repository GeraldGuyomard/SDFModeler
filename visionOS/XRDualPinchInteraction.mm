//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRDualPinchInteraction.h"
#include "XRDragInteraction.h"

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

float rotationAngle(const float3& leftPos, const float3& rightPos)
{
    const float2 leftPos2D { leftPos.x, leftPos.z };
    const float2 rightPos2D { rightPos.x, rightPos.z };
    const float2 v = normalize(leftPos2D - rightPos2D);
    
    return atan2(v.y, v.x);
}

XRDualPinchInteraction::ActivePayload::ActivePayload(float dist, const float3& leftHandPos, const float3& rightHandPos, const Object3D::Ptr& object)
: initialDistance(dist),
initialRotationAngle(rotationAngle(leftHandPos, rightHandPos)),
entry { object }
{}

XRInteraction::State
XRDualPinchInteraction::_updateWhenInactive(const XRHandAnchors& anchors)
{
    ASSERT(_activePayload == nullptr);
    
    const auto* closestEntry = anchors.closestEntryToAnyHand();
    if (closestEntry == nullptr)
    {
        return State::inactive;
    }
    
    // closest position but be very close to object
    if (closestEntry->distance > XRDragInteraction::kMinDistanceToAnyObjectForActivation)
    {
        return State::inactive;
    }
    
    size_t nbPinchingHands = 0;
    for (const auto& entry : anchors.entries())
    {
        const auto& anchor = entry.handAnchor;
        if ((anchor != nullptr) && anchor->isPinching())
        {
            ++nbPinchingHands;
        }
    }
    
    if (nbPinchingHands != 2)
    {
        return State::inactive;
    }
    
    const auto object = closestEntry->object;
    
    const auto posLeft = anchors.entry(Chirality::left).position;
    const auto posRight = anchors.entry(Chirality::right).position;
    
    const auto d = length(posLeft - posRight);
    if (d < kMinDistance)
    {
        return State::inactive;
    }
    
    _activePayload = std::make_unique<ActivePayload>(d, posLeft, posRight, object);
    
    return State::active;
}

XRInteraction::State
XRDualPinchInteraction::_updateWhenActive(const XRHandAnchors& anchors)
{
    ASSERT(_activePayload != nullptr);
    
    const auto& leftEntry = anchors.entry(Chirality::left);
    const auto& rightEntry = anchors.entry(Chirality::right);
    
    if ((leftEntry.handAnchor == nullptr) || (rightEntry.handAnchor == nullptr))
    {
        // lost tracking
        return State::active;
    }
    
    if (!leftEntry.handAnchor->isPinching() && !rightEntry.handAnchor->isPinching())
    {
        return State::inactive;
    }
    
    const auto posLeft = leftEntry.position;
    const auto posRight = rightEntry.position;
    
    const auto d = length(posLeft - posRight);
    
    if (d < kMinDistance)
    {
        return State::active;
    }
    
    auto& payload = *_activePayload;
    
    const float scale = 1.f + (d - payload.initialDistance) / d;
    //NSLog(@"scale=%5.3f newDist=%5.3f initialDistance=%5.3f", scale, d, payload.initialDistance);
    
    const auto pos = translation(payload.entry.transform);
    const auto moveToPivot = matrix4x4_translation(-pos);
    const auto moveBackToPos = matrix4x4_translation(pos);
    const auto scaleM = matrix4x4_scale(scale);
    
    const float newRotAngle = rotationAngle(posLeft, posRight);
    const float deltaRot = payload.initialRotationAngle - newRotAngle;
    
    const auto rot = matrix4x4_rotation(deltaRot, float3 {0.f, 1.f, 0.f });
    
    const auto transform = moveBackToPos * rot * scaleM * moveToPivot * payload.entry.transform;
    
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

