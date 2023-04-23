//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Camera.h"

LookAtObject3DProvider::LookAtObject3DProvider(const Object3D::Ptr& object)
: _object(object)
{}

std::optional<float3>
LookAtObject3DProvider::position() const
{
    if (auto object = _object.lock())
    {
        return translation(object->worldTransform());
    }
    else
    {
        return {};
    }
}

void
Camera::setWorldTransform(const float4x4& t)
{
    _worldTransform = t;
}

float3
Camera::lookAtPosition()
{
    std::optional<float3> pos;
    if (_lookAtPositionProvider != nullptr)
    {
        pos = _lookAtPositionProvider->position();
    }
    
    if (pos.has_value())
    {
        _defaultLookAtPosition = pos.value();
    }
    
    return _defaultLookAtPosition;
}

void
Camera::setLookAtPositionProvider(LookAtPositionProvider::Ptr provider)
{
    _lookAtPositionProvider = std::move(provider);
}

float3
Camera::computeOrbitOrigin()
{
    const auto cameraTransform = worldTransform();
    
    const auto position = cameraTransform.columns[3].xyz;
    const auto direction = simd_normalize(cameraTransform.columns[2].xyz);
    
    const float3 lookAtVector = lookAtPosition() - position;
    const float proj = dot(direction, lookAtVector);
    return position + (direction * proj);
}

void
Camera::setViewportSize(const float2& size)
{
    _viewportSize = size;
}

float4x4
Camera::computeProjectionMatrix() const
{
    const float aspect = _viewportSize.x / _viewportSize.y;
    return matrix_perspective_right_hand(_fovyRadians, aspect, _nearZ, _farZ);
}
