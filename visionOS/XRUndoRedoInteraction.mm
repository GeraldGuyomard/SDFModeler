//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRUndoRedoInteraction.h"

XRUndoRedoInteraction::XRUndoRedoInteraction(const WorldPtr& world, Type type)
: _world(world), _type(type)
{}

bool
XRUndoRedoInteraction::_isGestureDetected(const XRHandAnchor& anchor) const
{
    // thumbs down -> direction from wrist to thumb tip is vertical
    
    // The thumb should point down
    const auto wristPosition = translation(anchor.jointTransformInWorldSpace(JointID::wrist));
    const auto thumbPosition = translation(anchor.jointTransformInWorldSpace(JointID::thumbTip));
    
    const auto dir = normalize(thumbPosition - wristPosition);
    
    const float3 idealDir = (_type == Type::undo) ? float3 { 0.f, -1.f, 0.f } : float3 { 0.f, +1.f, 0.f };
    const float dotThumb = simd_dot(dir, idealDir);
    if (dotThumb < 0.9f)
    {
        return false;
    }
    
    // The 4 remaining fingers should be aligned vertically
    // and all not distant from the the axis
    const auto indexPosition = translation(anchor.jointTransformInWorldSpace(JointID::indexFingerTip));
    const auto middlePosition = translation(anchor.jointTransformInWorldSpace(JointID::middleFingerTip));
    const auto ringPosition = translation(anchor.jointTransformInWorldSpace(JointID::ringFingerTip));
    const auto littlePosition = translation(anchor.jointTransformInWorldSpace(JointID::littleFingerTip));
    
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
{
}

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
XRUndoRedoInteraction::_onStateChanged(State oldState, State newState)
{
    switch(newState)
    {
        case State::inactive: _tracking.reset(); break;
        default: break;
    }
}

XRInteraction::State
XRUndoRedoInteraction::_updateWhenInactive(const XRHandAnchors& anchors)
{
    for (const auto& entry : anchors.entries())
    {
        const auto& anchor = entry.handAnchor;
        if ((anchor != nullptr) && _isGestureDetected(*anchor))
        {
            _tracking = Tracking { anchor->chirality() };
            return State::possible;
        }
    }
    
    return State::inactive;
}

XRInteraction::State
XRUndoRedoInteraction::_updateWhenPossible(const XRHandAnchors& anchors)
{
    ASSERT(_tracking.has_value());
    
    const auto& tracking = _tracking.value();
    
    const auto& anchor = anchors.anchor(tracking.chirality());
    if (anchor == nullptr)
    {
        // lost tracking
        return State::possible;
    }
    
    if (!_isGestureDetected(*anchor))
    {
        return State::inactive;
    }
    
    if (tracking.enoughTimeElapsed())
    {
        return State::active;
    }
    
    return State::possible;
}

XRInteraction::State
XRUndoRedoInteraction::_updateWhenActive(const XRHandAnchors& anchors)
{
    if (_type == Type::undo)
    {
        _world->commandHistory().undo();
    }
    else
    {
        _world->commandHistory().redo();
    }
    
    return State::inactive;
}


XRInteraction::State
XRUndoRedoInteraction::update(const XRHandAnchors& anchors)
{
    switch (state())
    {
        case State::inactive: return _updateWhenInactive(anchors);
        case State::possible: return _updateWhenPossible(anchors);
        case State::active: return _updateWhenActive(anchors);
    }
}

