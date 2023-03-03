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
        const auto localP = _invRSTransform * float4 { p.x, p.y, p.z, 1.f };
        const auto transformedP = localP.xyz / localP.w;
        return primitive.computeDistance(transformedP);
    }
    
private:
    float4x4 _invRSTransform = float4x4_identity();
    float3 _scale = 1.f;
};
