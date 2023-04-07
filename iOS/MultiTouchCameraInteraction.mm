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

float2
MultiTouchCameraInteraction::TouchEntry::dragVector() const
{
    return currentLocation - initialLocation;
}

bool
MultiTouchCameraInteraction::TouchEntry::dragging() const
{
    const float d = length(dragVector());
    return d >= 10.f;
}

void
MultiTouchCameraInteraction::touchesBegan(NSSet<UITouch*>* touches)
{
    for (UITouch* touch : touches)
    {
        _uiTouchToEntry[touch] = touchLocation(touch);
        
        if (_firstTouch == nil)
        {
            _firstTouch = touch;
        }
        else if (_secondTouch == nil)
        {
            _secondTouch = touch;
        }
    }
       
    if (_state == State::idle)
    {
        _state = State::possible;
    }
}

void
MultiTouchCameraInteraction::touchesMoved(NSSet<UITouch*>* touches)
{
    for (UITouch* touch in touches)
    {
        const auto it = _uiTouchToEntry.find(touch);
        if (it != _uiTouchToEntry.end())
        {
            auto& entry = it->second;
            entry.currentLocation = touchLocation(touch);
            
            if (entry.dragging())
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
    if (_firstTouch == nil)
    {
        return;
    }
    
    if (_secondTouch != nil)
    {
        const auto& firstEntry = _uiTouchToEntry.find(_firstTouch)->second;
        const auto& secondEntry = _uiTouchToEntry.find(_secondTouch)->second;
        
        // dolly
        {
            const float2 initialVector = secondEntry.initialLocation - firstEntry.initialLocation;
            const float initialLength = length(initialVector);
            
            const float2 currentVector = secondEntry.currentLocation - firstEntry.currentLocation;
            const float currentLength = length(currentVector);
            
            _dollyFactor = (1.f - (currentLength / initialLength)) * kDefaultDollySpeed;
        }
        
        // pan
        {
            const float2 initialCentroid = (firstEntry.initialLocation + secondEntry.initialLocation) * 0.5f;
            const float2 currentCentroid = (firstEntry.currentLocation + secondEntry.currentLocation) * 0.5f;
            
            const float2 move = initialCentroid - currentCentroid;
            _panTranslation = move * 2e-3f;
            _panTranslation.y *= -1.f;
        }
    }
    else
    {
        const auto& firstEntry = _uiTouchToEntry.find(_firstTouch)->second;
        
        _orbitAngles = firstEntry.dragVector();
        _orbitAngles *= kDefaultOrbitSpeed;
    }
}

void
MultiTouchCameraInteraction::touchesEnded(NSSet<UITouch*>* touches)
{
    for (UITouch* touch in touches)
    {
        const auto it = _uiTouchToEntry.find(touch);
        if (it != _uiTouchToEntry.end())
        {
            _uiTouchToEntry.erase(it);
        }
        
        if (touch == _firstTouch)
        {
            _firstTouch = nil;
        }
        else if (touch == _secondTouch)
        {
            _secondTouch = nil;
        }
    }
    
    if (_uiTouchToEntry.empty())
    {
        _state = State::done;
    }
    else if ((_firstTouch == nil) && (_secondTouch != nil))
    {
        _firstTouch = _secondTouch;
        _secondTouch = nil;
    }
}

void
MultiTouchCameraInteraction::updateCameraTransform()
{
    if (_firstTouch == nil)
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
