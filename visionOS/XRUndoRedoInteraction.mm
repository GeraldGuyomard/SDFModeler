//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRUndoRedoInteraction.h"

XRUndoRedoInteraction::XRUndoRedoInteraction(World& world, Type type)
: _world(world), _type(type)
{
}


bool
XRUndoRedoInteraction::_isGestureDetected(const XRHandAnchor* anchor) const
{
    if (anchor == nullptr)
    {
        return false;
    }
    
    // thumbs down -> direction from wrist to thumb tip is vertical
    
    // The thumb should point down
    const auto wristPosition = translation(anchor->jointTransformInWorldSpace(JointID::wrist));
    const auto thumbPosition = translation(anchor->jointTransformInWorldSpace(JointID::thumbTip));
    
    const auto dir = normalize(thumbPosition - wristPosition);
    
    const float3 idealDir = (_type == Type::undo) ? float3 { 0.f, -1.f, 0.f } : float3 { 0.f, +1.f, 0.f };
    const float dotThumb = simd_dot(dir, idealDir);
    if (dotThumb < 0.9f)
    {
        return false;
    }
    
    // The 4 remaining fingers should be aligned vertically
    // and all not distant from the the axis
    const auto indexPosition = translation(anchor->jointTransformInWorldSpace(JointID::indexFingerTip));
    const auto middlePosition = translation(anchor->jointTransformInWorldSpace(JointID::middleFingerTip));
    const auto ringPosition = translation(anchor->jointTransformInWorldSpace(JointID::ringFingerTip));
    const auto littlePosition = translation(anchor->jointTransformInWorldSpace(JointID::littleFingerTip));
    
    const auto averageSecondaryPosition = (indexPosition + middlePosition + ringPosition + littlePosition) / 4.f;
    
    // distance point to line
    // https://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
    const auto averageSecondaryDir = averageSecondaryPosition - wristPosition;
    const float distSecToMainLine = length(cross(averageSecondaryDir, wristPosition - indexPosition)) / length(averageSecondaryDir);
    if (distSecToMainLine > 0.04f)
    {
        return false;
    }
    
    const auto secondaryDir = normalize(indexPosition - littlePosition);
    
    // Eliminate gesture if other fingers are not aligned, like hellfest gesture
    const auto thirdDir = normalize(middlePosition - ringPosition);
    
    const float dotSecondary = dot(secondaryDir, thirdDir);
    if (dotSecondary <= 0.5f)
    {
        return false;
    }
    
    return true;
}

XRUndoRedoInteraction::Tracking::Tracking(Chirality c)
: _chirality(c), _startTime(Clock::now())
{}

bool
XRUndoRedoInteraction::Tracking::enoughTimeElapsed() const
{
    const auto now = Clock::now();
    const float dT = std::chrono::duration_cast<std::chrono::milliseconds>(now - _startTime).count() / 1000.f;
    
    constexpr float kStillDuration = 1.5f;
    return dT >= kStillDuration;
}

void
XRUndoRedoInteraction::Tracking::resetTime()
{
    _startTime = Clock::now();
}

void
XRUndoRedoInteraction::update(const XRHandAnchor* left, const XRHandAnchor* right)
{
    // Undo is thumb down for a while

    if (_tracking.has_value())
    {
        auto& tracking = _tracking.value();
        const XRHandAnchor* anchor = (tracking.chirality() == Chirality::left) ? left : right;
        if (_isGestureDetected(anchor))
        {
            if (tracking.enoughTimeElapsed())
            {
                if (_type == Type::undo)
                {
                    _world.commandHistory().undo();
                }
                else
                {
                    _world.commandHistory().redo();
                }
                
                _tracking.reset();
            }
        }
        else
        {
            // keep tracking a still pose
            //tracking.resetTime();
        }
    }
    else if (_isGestureDetected(left))
    {
        _tracking = Tracking { Chirality::left };
    }
    else if (_isGestureDetected(right))
    {
        _tracking = Tracking { Chirality::right };
    }
}

