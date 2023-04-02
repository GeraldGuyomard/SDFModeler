//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "Camera.h"

class CameraController
{
public:
    virtual ~CameraController() = default;
    
    const Camera::Ptr& camera() const { return _camera; }
    
protected:
    CameraController(const Camera::Ptr&);
    
private:
    Camera::Ptr _camera;
};

class OrbitCameraController : public CameraController
{
public:
    using _inherited = CameraController;
    
    OrbitCameraController(const Camera::Ptr&, const float2& initialPos);
    
    void drag(const float2& pos);
    
private:
    const float2 _initialPos;
    const float4x4 _initialCameraTransform;
    const float3 _orbitOrigin;
};

class DollyCameraController : public CameraController
{
public:
    using _inherited = CameraController;
    
    DollyCameraController(const Camera::Ptr& camera, const float2& initialPos);
    
    void dolly(float delta);
    
private:
    const float2 _initialPos;
};

class PanCameraController : public CameraController
{
public:
    using _inherited = CameraController;
    
    PanCameraController(const Camera::Ptr&, const float2& initialPos);
    
    void pan(const float2& pos);
    
private:
    const float2 _initialPos;
    float3 _right;
    float3 _up;
};
