//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRUndoInteraction.h"

XRUndoInteraction::XRUndoInteraction(World& world)
: _world(world)
{
}

namespace
{
    bool isThumbDown(const XRHandAnchor* anchor)
    {
        // @ to fix
        return false;
        
        if (anchor == nullptr)
        {
            return false;
        }
        
        // Up should be horizontal
        // left for right hand
        // right for left hand
        const float aimedDirection = anchor->chirality() == Chirality::left ? 1.f : -1.f;
        const float3 dir { aimedDirection, 0.f, 0.f };
        
        const auto worldTransform = anchor->worldTransform();
        const auto u = up(worldTransform);
        
        const float dot = simd_dot(u, dir);
        if (dot <= 0.8f)
        {
            return false;
        }
        
        // The thumb should point down
        const auto thumbWorldTransform = anchor->jointTransformInWorldSpace(JointID::thumbTip);
        const auto thumbUp = up(thumbWorldTransform);
        
        const float3 downDir { 0.f, -1.f, 0.f };
        const float dotThumb = simd_dot(downDir, thumbUp);
        if (dotThumb < 0.8f)
        {
            return false;
        }
        
        // The 4 remaining fingers should be close to the origin of the hand
        
        
        return true;
    }
}

XRUndoInteraction::Tracking::Tracking(Chirality c)
: _chirality(c), _startTime(Clock::now())
{}

bool
XRUndoInteraction::Tracking::enoughTimeElapsed() const
{
    const auto now = Clock::now();
    const float dT = std::chrono::duration_cast<std::chrono::milliseconds>(now - _startTime).count() / 1000.f;
    
    constexpr float kStillDuration = 1.f;
    return dT >= kStillDuration;
    
}

void
XRUndoInteraction::update(const XRHandAnchor* left, const XRHandAnchor* right)
{
    // Undo is thumb down for a while

    if (_tracking.has_value())
    {
        const auto& tracking = _tracking.value();
        const XRHandAnchor* anchor = (tracking.chirality() == Chirality::left) ? left : right;
        if (isThumbDown(anchor))
        {
            if (tracking.enoughTimeElapsed())
            {
                _world.commandHistory().undo();
                _tracking.reset();
            }
        }
        else
        {
            // keep tracking a still pose
        }
    }
    else if (isThumbDown(left))
    {
        _tracking = Tracking { Chirality::left };
    }
    else if (isThumbDown(right))
    {
        _tracking = Tracking { Chirality::right };
    }
}

