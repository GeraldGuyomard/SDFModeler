//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "CameraInteraction.h"
#import <UIKit/UIKit.h>
#include <vector>

class Renderer;

class MultiTouchCameraInteraction : public Interaction, public CameraInteraction
{
public:
    using Ptr = std::shared_ptr<MultiTouchCameraInteraction>;
    
    static constexpr float kDefaultOrbitSpeed = 2e-3f;
    static constexpr float kDefaultDollySpeed = 2.f;
    
    MultiTouchCameraInteraction(const Camera::Ptr& camera, const Renderer* renderer, const float3& orbitOrigin);
    MultiTouchCameraInteraction(const Camera::Ptr& camera, const Renderer* renderer);
    ~MultiTouchCameraInteraction() override = default;
    
    enum class State
    {
        idle,
        possible,
        active,
        done
    };
    
    State state() const { return _state; }
    
    void touchesBegan(NSSet<UITouch*>* touches);
    void touchesMoved(NSSet<UITouch*>* touches);
    void touchesEnded(NSSet<UITouch*>* touches);
    
private:
    
    void updateCameraTransform();
    void reset();
    
    const Renderer* const _renderer;
    
    float4x4 _initialCameraTransform;
    const float3 _orbitOrigin;
    
    class TrackedTouch final
    {
    public:
        TrackedTouch(UITouch* touch);
        
        UITouch* touch() const { return _touch; }
        
        bool dragging() const;
        float2 dragVector() const;
        
        float2 currentLocation() const;
        float2 initialLocation() const { return _initialLocation; }
        
        void resetInitialLocation();
        
    private:
        UITouch* _touch;
        float2 _initialLocation = { 0 };
    };
    
    std::vector<std::unique_ptr<TrackedTouch>> _trackedTouches;
    
    State _state = State::idle;
    
    float2 _orbitAngles = { 0 };
    float _dollyFactor = 0.f;
    float2 _panTranslation = { 0 };
};
