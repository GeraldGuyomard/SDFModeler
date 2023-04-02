//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "CameraController.h"

CameraController::CameraController(const Camera::Ptr& camera)
: _camera(camera)
{}

namespace
{

float3 computeOrbitOrigin(const float4x4& cameraTransform)
{
    const auto position = cameraTransform.columns[3].xyz;
    const auto direction = simd_normalize(cameraTransform.columns[2].xyz);
    
    return position - (direction * 5.f);
}

}

OrbitCameraController::OrbitCameraController(const Camera::Ptr& camera, const float2& initialPos, float speed)
: _inherited(camera),
_initialPos(initialPos),
_initialCameraTransform(camera->worldTransform()),
_orbitOrigin(computeOrbitOrigin(_initialCameraTransform)),
_speed(speed)
{
}

void
OrbitCameraController::orbit(const float2& pt)
{
    simd_float2 delta = pt - _initialPos;
    
    // yaw
    const auto yaw = matrix4x4_rotation(-delta.x * _speed, float3 { 0, 1, 0 }, _orbitOrigin);
    
    // pitch on new right axis
    const auto transformAfterYaw = yaw * _initialCameraTransform;
    const float3 pitchAxis = right(transformAfterYaw);
    const auto pitch = matrix4x4_rotation(-delta.y * _speed, pitchAxis, _orbitOrigin);
    
    const auto newTransform = pitch * transformAfterYaw;
    
    camera()->setWorldTransform(newTransform);
}

DollyCameraController::DollyCameraController(const Camera::Ptr& camera)
: _inherited(camera)
{}

void
DollyCameraController::dolly(float delta)
{
    auto camera = this->camera();
    
    auto transform = camera->worldTransform();
    auto pos = translation(transform);
    
    pos += delta * forward(transform);
    setTranslation(transform, pos);
    
    camera->setWorldTransform(transform);
}

PanCameraController::PanCameraController(const Camera::Ptr& camera, const float2& initialPos)
: _inherited(camera), _previousPos(initialPos)
{
    auto decomp = decompose(camera->worldTransform());
    _right = decomp.right;
    _up = decomp.up;
}

void
PanCameraController::pan(const float2& pos)
{
    float2 delta = pos - _previousPos;
    constexpr float k = -2e-3f;
    delta *= k;
    
    _previousPos = pos;
    
    auto camera = this->camera();
    float4x4 worldTransform = camera->worldTransform();
    
    float3 cameraPos = translation(worldTransform);
    cameraPos += _right * delta.x;
    cameraPos += _up * -delta.y;
    
    setTranslation(worldTransform, cameraPos);
    
    camera->setWorldTransform(worldTransform);
}
