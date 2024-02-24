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

class Camera final : public Object3D
{
public:
    using Ptr = std::shared_ptr<Camera>;
    using _inherited = Object3D;
    
    Camera(const WorldPtr&);
    
    float fovyRadians() const { return _fovyRadians; }
    float fovxRadians() const;
    float2 fovRadians() const { return { fovxRadians(), fovyRadians() }; }
    
    float aspectRatio() const;
    
    void setViewportSize(const float2&);
    float4x4 computeProjectionMatrix() const;
    
    float3 computeFramePosition(const Object3D::Ptr& object) const;
    float4x4 computeFrameTransform(const Object3D::Ptr& object) const;
    
private:
    
    // Intrinsics
    float _fovyRadians = 45.0f * (M_PI / 180.0f);
    float2 _viewportSize = { 1.f, 1.f };
    float _nearZ = 0.1f;
    float _farZ = 40.f;
};
