//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Camera.h"
#import <Spatial/Spatial.h>

Camera::Camera(const WorldPtr& world)
: _inherited(world)
{}

void
Camera::setViewportSize(const float2& size)
{
    _viewportSize = size;
}

CameraIntrinsics::CameraIntrinsics():
_nearZ(0.1f),
_farZ(std::numeric_limits<float>::infinity())
//_farZ(100.f)
{}

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

float2
FOVCameraIntrinsics::fovRadians(const float2& viewportSize) const
{
    return { fovxRadians(viewportSize), fovyRadians() };
}

float2
TangentsCameraIntrinsics::fovRadians(const float2& viewportSize) const
{
    const float nearZ = this->nearZ();
    const float height = _tangents[2];
    const float tanY = (height * 0.5f) / nearZ;
    
    // @todo fix this hack
    //const float fovY = atanf(tanY) * 2.f;
    const float fovY = atanf(tanY) * 0.7f;
    
    const float aspectRatio = viewportSize.x / viewportSize.y;
    
    const float fovX =  2.f * atanf(tanf(fovY / 2.f)) * aspectRatio;
    
    return { fovX, fovY };
}

float4x4
TangentsCameraIntrinsics::computeProjectionMatrix(const float2& viewportSize, bool inverseZ) const
{
    const auto proj = matrix_perspective(_tangents[0],
                              _tangents[1],
                              _tangents[2],
                              _tangents[3],
                              nearZ(),
                              farZ(),
                              inverseZ);
    
    return proj;
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
