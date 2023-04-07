//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "Camera.h"
#include "Interaction.h"
#include "Object3D.h"

class CameraInteraction
{
public:
    const Camera::Ptr& camera() const { return _camera; }
    
protected:
    CameraInteraction(const Camera::Ptr&);
    ~CameraInteraction() = default;
    
private:
    Camera::Ptr _camera;
};

class OrbitCameraInteraction : public PanInteraction, public CameraInteraction
{
public:
    static constexpr float kDefaultSpeed = 1e-3f;
    OrbitCameraInteraction(const Camera::Ptr&, const float2& initialPos, float speed = kDefaultSpeed);
    OrbitCameraInteraction(const Camera::Ptr&, const float3& origin, const float2& initialPos, float speed = kDefaultSpeed);
    
    void pan(const float2& pos) override;
    
    static float3 computeOrbitOrigin(const float4x4& cameraTransform);
    
private:
    const float4x4 _initialCameraTransform;
    const float3 _orbitOrigin;
    const float _speed;
};

class DollyCameraInteraction : public PinchInteraction, public CameraInteraction
{
public:
    using Ptr = std::shared_ptr<DollyCameraInteraction>;
    
    DollyCameraInteraction(const Camera::Ptr& camera);
    DollyCameraInteraction(const Camera::Ptr& camera, const Object3D::Ptr& target);
    
    void pinch(float delta) override;
    
private:
    const float3 _direction;
};

class PanCameraInteraction : public PanInteraction, public CameraInteraction
{
public:
    PanCameraInteraction(const Camera::Ptr&, const float2& initialPos);
    
    void pan(const float2& pos) override;
    
private:
    float2 _previousPos;
    float3 _right;
    float3 _up;
};

