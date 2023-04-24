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

class LookAtPositionProvider
{
public:
    using Ptr = std::unique_ptr<LookAtPositionProvider>;
    virtual ~LookAtPositionProvider() = default;
    
    virtual std::optional<float3> position() const = 0;
};

class LookAtObject3DProvider : public LookAtPositionProvider
{
public:
    LookAtObject3DProvider(const Object3D::Ptr& object);
    
    std::optional<float3> position() const override;
    
private:
    Object3D::WPtr _object;
};

class Camera final
{
public:
    using Ptr = std::shared_ptr<Camera>;
    
    Camera() = default;
    
    const float4x4& worldTransform() const { return _worldTransform; }
    void setWorldTransform(const float4x4&);
    
    float fovyRadians() const { return _fovyRadians; }
    float fovxRadians() const;
    float2 fovRadians() const { return { fovxRadians(), fovyRadians() }; }
    
    float aspectRatio() const;
    
    float3 lookAtPosition();
    void setLookAtPositionProvider(LookAtPositionProvider::Ptr);
    
    float3 computeOrbitOrigin();
    
    void setViewportSize(const float2&);
    float4x4 computeProjectionMatrix() const;
    
    float3 computeFramePosition(const Object3D::Ptr& object) const;
    
private:
    float4x4 _worldTransform = float4x4_identity();
    LookAtPositionProvider::Ptr _lookAtPositionProvider;
    float3 _defaultLookAtPosition = float3 { 0.f };
    
    // Intrinsics
    float _fovyRadians = 45.0f * (M_PI / 180.0f);
    float2 _viewportSize = { 1.f, 1.f };
    float _nearZ = 0.1f;
    float _farZ = 40.f;
};
