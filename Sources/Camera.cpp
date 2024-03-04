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
TangentsCameraIntrinsics::fovRadians(const float2&) const
{
    const float width = _tangents[0] + _tangents[1];
    const float height = _tangents[2] + _tangents[3];
    
    const float nearZ = this->nearZ();
    
    const float tanX = (width * 0.5f) / nearZ;
    const float tanY = (height * 0.5f) / nearZ;
    
    const float fovX = atanf(tanX);
    const float fovY = atanf(tanY);
    
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

template <typename TOut, typename TIn> TOut convert(TIn in)
{
    TOut out;
    
    for (size_t y=0; y < 4; ++y)
    {
        for (size_t x=0; x < 4; ++x)
        {
            out.columns[x][y] = in.columns[x][y];
        }
    }
    
    return out;
}

void
Camera::setProjectionMatrix(const float4x4& projMatrix)
{
    _projectionMatrix = projMatrix;
    
    //
    double4x4 p = convert<double4x4, float4x4>(projMatrix);
    const double d = determinant(p);
    p = inverse(p);
    const double d2 = determinant(p);
    
    _invProjectionMatrix = convert<float4x4, double4x4>(p);
}
