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

OrbitCameraController::OrbitCameraController(const Camera::Ptr& camera, const float2& initialPos)
: _inherited(camera),
_initialPos(initialPos),
_initialCameraTransform(camera->worldTransform()),
_orbitOrigin(computeOrbitOrigin(_initialCameraTransform))
{
}

void
OrbitCameraController::orbit(const float2& pt)
{
    simd_float2 delta = pt - _initialPos;
    
    auto decomp = decompose(_initialCameraTransform);
    
    // yaw
    const auto yaw = matrix4x4_rotation(-delta.x * 1e-3f, float3 { 0, 1, 0 }, _orbitOrigin);
    
    auto newPos = yaw * make_float4(decomp.position, 1.f);
    decomp.position = newPos.xyz;
    
    decomp.forward = normalize(decomp.position - _orbitOrigin);
    decomp.right = cross(decomp.up, decomp.forward);
    
    auto newTransform = recompose(decomp);
    
    // pitch
    decomp = decompose(newTransform);
    
    const auto pitch = matrix4x4_rotation(-delta.y * 1e-3f, float3 { 1, 0, 0 }, _orbitOrigin);
    
    newPos = pitch * make_float4(decomp.position, 1.f);
    decomp.position = newPos.xyz;
    
    decomp.forward = normalize(decomp.position - _orbitOrigin);
    decomp.up = (yaw * make_float4(decomp.up, 0.f)).xyz;
    
    newTransform = recompose(decomp);
    
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
    constexpr float k = -1.f / 1000.f;
    delta *= k;
    
    _previousPos = pos;
    
    auto camera = this->camera();
    float4x4 worldTransform = camera->worldTransform();
    
    float3 cameraPos = translation(worldTransform);
    cameraPos += _right * delta.x;
    cameraPos += _up * delta.y;
    
    setTranslation(worldTransform, cameraPos);
    
    camera->setWorldTransform(worldTransform);
}
