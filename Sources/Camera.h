//
//  Camera.hpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include <memory>
#include "CommonDefinitions.h"

class Camera final
{
public:
    using Ptr = std::shared_ptr<Camera>;
    
    Camera() = default;
    
    const float4x4& worldTransform() const { return _worldTransform; }
    void setWorldTransform(const float4x4&);
    
    float3 lookAtPosition() const { return _lookAtPosition; }
    void setLookAtPosition(float3);
    
    float3 computeOrbitOrigin() const;
    
private:
    float4x4 _worldTransform = float4x4_identity();
    float3 _lookAtPosition = { 0 };
};
