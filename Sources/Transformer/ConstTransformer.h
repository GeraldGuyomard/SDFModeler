//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "Transformer/Transformer.h"

class ConstTransformer final
{
public:
    
    ConstTransformer() = default;
    
    ConstTransformer(float3 translation)
    : _invRSTransform(matrix4x4_translation(-translation))
    {}
    
    ConstTransformer(float3 translation, float3 rotationAxis, float angle, float scale = 1.f)
    : _invRSTransform(matrix4x4_rotation(-angle, rotationAxis) * matrix4x4_translation(-translation)), _scale(scale)
    {}
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        float invScale = 1.f / _scale;
        p *= invScale;
        
        const auto localP = _invRSTransform * float4 { p.x, p.y, p.z, invScale };
        const auto transformedP = localP.xyz / localP.w;
        return primitive.computeDistance(transformedP) * _scale;
    }
    
private:
    float4x4 _invRSTransform = float4x4_identity();
    float _scale = 1.f;
};
