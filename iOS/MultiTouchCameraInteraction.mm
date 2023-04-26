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
_orbitOrigin(camera->computeOrbitOrigin())
{}

void
MultiTouchCameraInteraction::setOrbitSpeed(float orbitSpeed)
{
    _orbitSpeed = orbitSpeed;
}

float2 previousTouchLocation(UITouch* touch)
{
    const CGPoint loc = [touch previousLocationInView:nil];
    
    return float2 { float(loc.x), float(loc.y) };
}

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
MultiTouchCameraInteraction::TrackedTouch::previousLocation() const
{
    return previousTouchLocation(_touch);
}

float2
MultiTouchCameraInteraction::TrackedTouch::currentLocation() const
{
    return touchLocation(_touch);
}

bool
MultiTouchCameraInteraction::TrackedTouch::dragging() const
{
    const auto dist = currentLocation() - initialLocation();
    const float d = length(dist);
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
    _lastOrbitTime = OrbitClock::now();
    _lastOrbitDrag = { 0 };
    
    _dollyFactor = 0.f;
    _panTranslation = { 0 };
    
    for (const auto& touch : _trackedTouches)
    {
        touch->resetInitialLocation();
    }
}

namespace
{

float4x4 computeTransformAfterOrbit(float2 orbitAngles, float3 orbitOrigin, const float4x4& initialCameraTransform)
{
    const auto yaw = matrix4x4_rotation(-orbitAngles.x, float3 { 0, 1, 0 }, orbitOrigin);
    
    // pitch on new right axis
    const auto transformAfterYaw = yaw * initialCameraTransform;
    const float3 pitchAxis = right(transformAfterYaw);
    const auto pitch = matrix4x4_rotation(-orbitAngles.y, pitchAxis, orbitOrigin);
    
    return pitch * transformAfterYaw;
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
    const auto& firstTouch = _trackedTouches[0];
    
    if (_trackedTouches.size() >= 2)
    {
        const auto& secondTouch = _trackedTouches[1];
        
        // dolly
        const float2 initialVector = secondTouch->initialLocation() - firstTouch->initialLocation();
        const float initialLength = length(initialVector);
        
        const float2 currentVector = secondTouch->currentLocation() - firstTouch->currentLocation();
        const float currentLength = length(currentVector);
        
        const float3 pos = translation(newTransform);
        const float distanceToOrbitOrigin = length(pos - _orbitOrigin);
        const float distanceDampening = expf(distanceToOrbitOrigin * 0.1f);
       
        _dollyFactor = (initialLength - currentLength) * 0.01f * distanceDampening;
    }
    else
    {
        _lastOrbitTime = OrbitClock::now();
        
        _lastOrbitDrag = (firstTouch->currentLocation() - firstTouch->previousLocation());
        
        _orbitAngles = (firstTouch->currentLocation() - firstTouch->initialLocation());
        _orbitAngles *= _orbitSpeed;
    }
    
    // orbit
    {
        newTransform = computeTransformAfterOrbit(_orbitAngles, _orbitOrigin, _initialCameraTransform);
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
        if (_trackedTouches.size() >= 2)
        {
            const auto& secondTouch = _trackedTouches[1];
            
            const float2 previousCentroid = (firstTouch->previousLocation() + secondTouch->previousLocation()) * 0.5f;
            const float2 currentCentroid = (firstTouch->currentLocation() + secondTouch->currentLocation()) * 0.5f;
            float2 delta = currentCentroid - previousCentroid;
            delta.y *= -1.f;
            
            const float3 pos = translation(newTransform);
            const float distanceToOrbitOrigin = length(pos - _orbitOrigin);
            
            const float r = distanceToOrbitOrigin / 1e3f;
            _panTranslation -= delta * r;
        }
        
        const float3 upVector = up(newTransform);
        const float3 rightVector = right(newTransform);
        
        auto pos = translation(newTransform);
        
        pos += rightVector * _panTranslation.x;
        pos += upVector * _panTranslation.y;
        
        setTranslation(newTransform, pos);
    }
    
    camera()->setWorldTransform(newTransform);
}

class OrbitDecelerationAnimation final : public Animation
{
public:
    
    OrbitDecelerationAnimation(const Camera::Ptr& camera, float3 orbitOrigin, float2 orbitVelocity)
    : _camera(camera),
    _orbitOrigin(orbitOrigin),
    _orbitVelocity(orbitVelocity)
    {}
    
    bool isFinished() const override
    {
        return _dampening < 1e-2f;
    }
    
    void start(float t) override
    {
        _startT = t;
    }
    
    void update(float t) override
    {
        const float dT = t - _startT;
        _dampening = expf(-dT * 2.f);
        
        const float2 orbitAngles = _orbitVelocity * _dampening;
        
        const auto currentTransform = _camera->worldTransform();
        const auto newTransform = computeTransformAfterOrbit(orbitAngles, _orbitOrigin, currentTransform);
        _camera->setWorldTransform(newTransform);
    }
    
private:
    const Camera::Ptr _camera;
    const float2 _orbitVelocity;
    const float3 _orbitOrigin;
    
    float _startT = 0.f;
    float _dampening = 1.f;
};

Animation::Ptr
MultiTouchCameraInteraction::makeOrbitDecelerationAnimation() const
{
    const auto now = OrbitClock::now();
    auto dT = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastOrbitTime).count() / 1000.f;
    dT = max(dT, 1.f / 60.f);
    
    const float2 velocity = _lastOrbitDrag / dT;
    const float v = length(velocity);
    
    if (v <= 30.f)
    {
        return nullptr;
    }
    
    float2 orbitAngles = normalize(_orbitAngles) * v;
    orbitAngles.x *= 1e-4f;
    orbitAngles.y *= 0.5e-4f;
    
    return std::make_shared<OrbitDecelerationAnimation>(camera(), _orbitOrigin, orbitAngles);
}
