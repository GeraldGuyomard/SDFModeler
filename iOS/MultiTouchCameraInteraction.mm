//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MultiTouchCameraInteraction.h"
#include "SDFPlane.h"
#include "Renderer.h"

MultiTouchCameraInteraction::MultiTouchCameraInteraction(const Camera::Ptr& camera, const Renderer* renderer)
: CameraInteraction(camera),
_renderer(renderer),
_initialCameraTransform(camera->worldTransform()),
_orbitOrigin(OrbitCameraInteraction::computeOrbitOrigin(camera->worldTransform()))
{}

MultiTouchCameraInteraction::MultiTouchCameraInteraction(const Camera::Ptr& camera, const Renderer* renderer, const float3& orbitOrigin)
: CameraInteraction(camera),
_renderer(renderer),
_initialCameraTransform(camera->worldTransform()),
_orbitOrigin(orbitOrigin)
{}

float2 touchLocation(UITouch* touch)
{
    const CGPoint loc = [touch locationInView:nil];
    
    return float2 { float(loc.x), float(loc.y) };
}

MultiTouchCameraInteraction::TrackedTouch::TrackedTouch(UITouch* touch)
: _touch(touch), _initialLocation(touchLocation(touch))
{}

void
MultiTouchCameraInteraction::TrackedTouch::resetInitialLocation()
{
    _initialLocation = touchLocation(_touch);
}

float2
MultiTouchCameraInteraction::TrackedTouch::currentLocation() const
{
    return touchLocation(_touch);
}

float2
MultiTouchCameraInteraction::TrackedTouch::dragVector() const
{
    return currentLocation() - _initialLocation;
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
    const bool wasTrackingTwoTouches = _trackedTouches.size() >= 2;
    
    for (UITouch* touch : touches)
    {
        _trackedTouches.push_back(std::make_unique<TrackedTouch>(touch));
    }
       
    if (_state == State::idle)
    {
        _state = State::possible;
    }
    
    if (!wasTrackingTwoTouches)
    {
        reset();
    }
}

void
MultiTouchCameraInteraction::touchesMoved(NSSet<UITouch*>* touches)
{
    for (const auto& trackedTouch : _trackedTouches)
    {
        if ([touches containsObject:trackedTouch->touch()])
        {
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
        updateCameraTransform();
    }
}

void
MultiTouchCameraInteraction::touchesEnded(NSSet<UITouch*>* touches)
{
    bool touchesRemoved = false;
    for (auto it = _trackedTouches.begin(); it != _trackedTouches.end();)
    {
        const TrackedTouch& touch = *(*it);
        if ([touches containsObject:touch.touch()])
        {
            it = _trackedTouches.erase(it);
            touchesRemoved = true;
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
    else if (touchesRemoved)
    {
        reset();
    }
}

void
MultiTouchCameraInteraction::reset()
{
    _initialCameraTransform = camera()->worldTransform();
    _orbitAngles = { 0 };
    _dollyFactor = 0.f;
    _panTranslation = { 0 };
    
    for (const auto& touch : _trackedTouches)
    {
        touch->resetInitialLocation();
    }
}

void
MultiTouchCameraInteraction::updateCameraTransform()
{
    if (_trackedTouches.empty())
    {
        return;
    }
    
    const auto& touch0 = *_trackedTouches[0];
    const float2 initialLocation0 = touch0.initialLocation();
    const float2 currentLocation0 = touch0.currentLocation();
    
    if (_trackedTouches.size() >= 2)
    {
        const auto& touch1 = *_trackedTouches[1];
        const float2 initialLocation1 = touch1.initialLocation();
        const float2 currentLocation1 = touch1.currentLocation();
        
        // dolly
        {
            const float2 initialVector = initialLocation1 - initialLocation0;
            const float initialLength = length(initialVector);
            
            const float2 currentVector = currentLocation1 - currentLocation0;
            const float currentLength = length(currentVector);
            
            _dollyFactor = (1.f - (currentLength / initialLength)) * kDefaultDollySpeed;
        }
        
        // pan
        {
            const float2 initialCentroid = (initialLocation0 + initialLocation1) * 0.5f;
            
            SDFPlane dragPlane;
            
            const float2 currentCentroid = (currentLocation0 + currentLocation1) * 0.5f;
            
            
            const float2 move = initialCentroid - currentCentroid;
            _panTranslation = move * 2e-3f;
            _panTranslation.y *= -1.f;
        }
    }
    else
    {
        _orbitAngles = touch0.dragVector();
        _orbitAngles *= kDefaultOrbitSpeed;
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
