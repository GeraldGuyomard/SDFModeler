//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Camera.h"

Camera::Camera(const WorldPtr& world)
: _inherited(world)
{}

void
Camera::setViewportSize(const float2& size)
{
    _viewportSize = size;
}

float
Camera::aspectRatio() const
{
    return _viewportSize.x / _viewportSize.y;
}

float4x4
Camera::computeProjectionMatrix(const float2& viewportSizeInPoints) const
{
    const float aspectRatio = viewportSizeInPoints.x / viewportSizeInPoints.y;
    
    return matrix_perspective_right_hand(_fovyRadians, aspectRatio, _nearZ, _farZ);
}

float
Camera::fovxRadians() const
{
    return 2.f * atanf(tanf(_fovyRadians / 2.f)) * aspectRatio();
}
