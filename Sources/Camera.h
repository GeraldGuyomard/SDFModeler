//
//  Camera.hpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include <memory>
#include "CommonDefinitions.h"
#include "Object3D.h"
#include <optional>

class CameraIntrinsics
{
public:
    using Ptr = std::unique_ptr<CameraIntrinsics>;
    
    CameraIntrinsics() = default;
    virtual ~CameraIntrinsics() = default;
    
    virtual float4x4 computeProjectionMatrix(const float2& viewportSize, bool inverseZ) const = 0;

    float nearZ() const { return _nearZ; }
    void setNearZ(float z) { _nearZ = z; }
    
    float farZ() const { return _farZ; }
    void setFarZ(float z) { _farZ = z; }
    
private:

    float _nearZ = 0.1f;
    float _farZ = std::numeric_limits<float>::infinity();
};

class FOVCameraIntrinsics final : public CameraIntrinsics
{
public:
    using Ptr = std::unique_ptr<CameraIntrinsics>;
    
    float fovyRadians() const { return _fovyRadians; }
    float fovxRadians(const float2& viewportSize) const;
    float2 fovRadians(const float2& viewportSize) const { return { fovxRadians(viewportSize), fovyRadians() }; }
    
    float4x4 computeProjectionMatrix(const float2& viewportSize, bool inverseZ) const override;

private:
    
    float _fovyRadians = 45.0f * (M_PI / 180.0f);
};

class Camera final : public Object3D
{
public:
    using Ptr = std::shared_ptr<Camera>;
    using _inherited = Object3D;
    
    Camera(const WorldPtr&);
    
    const CameraIntrinsics* intrinsics() const { return _intrinsics.get(); }
    CameraIntrinsics* intrinsics() { return _intrinsics.get(); }
    void setIntrinsics(CameraIntrinsics::Ptr);
    
    const float2& viewportSize() const { return _viewportSize; }
    void setViewportSize(const float2&);
    float aspectRatio() const;
    
    const float4x4& projectionMatrix() const { return _projectionMatrix; }
    const float4x4& invProjectionMatrix() const { return _invProjectionMatrix; }
    void setProjectionMatrix(const float4x4&);
    
private:
    CameraIntrinsics::Ptr _intrinsics;
    float2 _viewportSize = { 0.f, 0.f };
    
    float4x4 _projectionMatrix = float4x4_identity();
    float4x4 _invProjectionMatrix = float4x4_identity();
};
