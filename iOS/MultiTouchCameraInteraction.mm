//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MultiTouchCameraInteraction.h"

MultiTouchCameraInteraction::MultiTouchCameraInteraction(const Camera::Ptr& camera, UIView* view)
: CameraInteraction(camera),
_view(view),
_initialCameraTransform(camera->worldTransform()),
_orbitOrigin(OrbitCameraInteraction::computeOrbitOrigin(camera->worldTransform()))
{}

MultiTouchCameraInteraction::MultiTouchCameraInteraction(const Camera::Ptr& camera, UIView* view, const float3& orbitOrigin)
: CameraInteraction(camera),
_view(view),
_initialCameraTransform(camera->worldTransform()),
_orbitOrigin(orbitOrigin)
{}

float2 touchLocation(UITouch* touch)
{
    const CGPoint loc = [touch locationInView:nil];
    
    return float2 { float(loc.x), float(loc.y) };
}

MultiTouchCameraInteraction::TrackedTouch::TrackedTouch(UITouch* touch)
: touch(touch), initialLocation(touchLocation(touch))
{
    currentLocation = initialLocation;
}

float2
MultiTouchCameraInteraction::TrackedTouch::dragVector() const
{
    return currentLocation - initialLocation;
}

bool
MultiTouchCameraInteraction::TrackedTouch::dragging() const
{
    const float d = length(dragVector());
    return d >= 10.f;
}

void
MultiTouchCameraInteraction::touchesBegan(NSSet<UITouch*>* touches)
{
    for (UITouch* touch : touches)
    {
        _trackedTouches.push_back(std::make_unique<TrackedTouch>(touch));
    }
       
    if (_state == State::idle)
    {
        _state = State::possible;
    }
}

void
MultiTouchCameraInteraction::touchesMoved(NSSet<UITouch*>* touches)
{
    for (const auto& trackedTouch : _trackedTouches)
    {
        if ([touches containsObject:trackedTouch->touch])
        {
            trackedTouch->currentLocation = touchLocation(trackedTouch->touch);
            
            if (trackedTouch->dragging())
            {
                if (_state == State::possible)
                {
                    _state = State::active;
                }
            }
        }
    }
    
    if (_state == State::active)
    {
        onDrag();
        updateCameraTransform();
    }
}

void
MultiTouchCameraInteraction::onDrag()
{
    if (_trackedTouches.empty())
    {
        return;
    }
    
    const auto& firstTouch = *_trackedTouches[0];
    
    if (_trackedTouches.size() >= 2)
    {
        const auto& secondTouch = *_trackedTouches[1];
        
        // dolly
        {
            const float2 initialVector = secondTouch.initialLocation - firstTouch.initialLocation;
            const float initialLength = length(initialVector);
            
            const float2 currentVector = secondTouch.currentLocation - firstTouch.currentLocation;
            const float currentLength = length(currentVector);
            
            _dollyFactor = (1.f - (currentLength / initialLength)) * kDefaultDollySpeed;
        }
        
        // pan
        {
            const float2 initialCentroid = (firstTouch.initialLocation + secondTouch.initialLocation) * 0.5f;
            const float2 currentCentroid = (firstTouch.currentLocation + secondTouch.currentLocation) * 0.5f;
            
            const float2 move = initialCentroid - currentCentroid;
            _panTranslation = move * 2e-3f;
            _panTranslation.y *= -1.f;
        }
    }
    else
    {
        _orbitAngles = firstTouch.dragVector();
        _orbitAngles *= kDefaultOrbitSpeed;
    }
}

void
MultiTouchCameraInteraction::touchesEnded(NSSet<UITouch*>* touches)
{
    for (auto it = _trackedTouches.begin(); it != _trackedTouches.end();)
    {
        const TrackedTouch& touch = *(*it);
        if ([touches containsObject:touch.touch])
        {
            it = _trackedTouches.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    if (_trackedTouches.empty())
    {
        _state = State::done;
    }
}

void
MultiTouchCameraInteraction::updateCameraTransform()
{
    if (_trackedTouches.empty())
    {
        return;
    }
    
    auto newTransform = _initialCameraTransform;
    
    // orbit
    {
        // yaw
        const auto yaw = matrix4x4_rotation(-_orbitAngles.x, float3 { 0, 1, 0 }, _orbitOrigin);
        
        // pitch on new right axis
        const auto transformAfterYaw = yaw * _initialCameraTransform;
        const float3 pitchAxis = right(transformAfterYaw);
        const auto pitch = matrix4x4_rotation(-_orbitAngles.y, pitchAxis, _orbitOrigin);
        
        newTransform = pitch * transformAfterYaw;
    }
    
    // dolly
    {
        auto pos = translation(newTransform);
        const float3 direction = forward(newTransform);
        
        pos += _dollyFactor * direction;
        setTranslation(newTransform, pos);
    }
    
    // pan
    {
        const float3 upVector = up(newTransform);
        const float3 rightVector = right(newTransform);
        
        auto pos = translation(newTransform);
        
        pos += rightVector * _panTranslation.x;
        pos += upVector * _panTranslation.y;
        
        setTranslation(newTransform, pos);
    }
    
    camera()->setWorldTransform(newTransform);
}
