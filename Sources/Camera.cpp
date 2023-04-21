//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Camera.h"

void
Camera::setWorldTransform(const float4x4& t)
{
    _worldTransform = t;
}

void
Camera::setLookAtPosition(float3 lookAtPos)
{
    _lookAtPosition = lookAtPos;
}

float3
Camera::computeOrbitOrigin() const
{
    const auto cameraTransform = worldTransform();
    
    const auto position = cameraTransform.columns[3].xyz;
    const auto direction = simd_normalize(cameraTransform.columns[2].xyz);
    
    const float3 lookAtVector = lookAtPosition() - position;
    const float proj = dot(direction, lookAtVector);
    return position + (direction * proj);
}
