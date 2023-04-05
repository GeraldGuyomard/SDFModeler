//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "Camera.h"

class CameraInteraction
{
public:
    using Ptr = std::unique_ptr<CameraInteraction>;
    
    virtual ~CameraInteraction() = default;
    
    const Camera::Ptr& camera() const { return _camera; }
    
protected:
    CameraInteraction(const Camera::Ptr&);
    
private:
    Camera::Ptr _camera;
};

class OrbitCameraInteraction : public CameraInteraction
{
public:
    using _inherited = CameraInteraction;
    
    OrbitCameraInteraction(const Camera::Ptr&, const float2& initialPos, float speed = 1e-3f);
    
    void orbit(const float2& pos);
    
private:
    const float2 _initialPos;
    const float4x4 _initialCameraTransform;
    const float3 _orbitOrigin;
    const float _speed;
};

class DollyCameraInteraction : public CameraInteraction
{
public:
    using _inherited = CameraInteraction;
    
    DollyCameraInteraction(const Camera::Ptr& camera);
    
    void dolly(float delta);
};

class PanCameraInteraction : public CameraInteraction
{
public:
    using _inherited = CameraInteraction;
    
    PanCameraInteraction(const Camera::Ptr&, const float2& initialPos);
    
    void pan(const float2& pos);
    
private:
    float2 _previousPos;
    float3 _right;
    float3 _up;
};
