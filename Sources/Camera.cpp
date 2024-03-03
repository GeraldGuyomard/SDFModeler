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

float4x4
FOVCameraIntrinsics::computeProjectionMatrix(const float2& viewportSize, bool inverseZ) const
{
    const float aspectRatio = viewportSize.x / viewportSize.y;
    
    return matrix_perspective_right_hand(_fovyRadians, aspectRatio, nearZ(), farZ());
}

float
FOVCameraIntrinsics::fovxRadians(const float2& viewportSize) const
{
    const float aspectRatio = viewportSize.x / viewportSize.y;
    
    return 2.f * atanf(tanf(_fovyRadians / 2.f)) * aspectRatio;
}

float4x4
TangentsCameraIntrinsics::computeProjectionMatrix(const float2& viewportSize, bool inverseZ) const
{
    return matrix_perspective(_tangents[0],
                              _tangents[1],
                              _tangents[2],
                              _tangents[3],
                              nearZ(),
                              farZ(),
                              inverseZ);
}

void
Camera::setIntrinsics(CameraIntrinsics::Ptr intrinsics)
{
    _intrinsics = std::move(intrinsics);
}

float
Camera::aspectRatio() const
{
    return _viewportSize.x / _viewportSize.y;
}

void
Camera::setProjectionMatrix(const float4x4& projMatrix)
{
    _projectionMatrix = projMatrix;
    _invProjectionMatrix = inverse(_projectionMatrix);
}
