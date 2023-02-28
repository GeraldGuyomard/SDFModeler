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
    
    ConstTransformer(float4x4 transform)
    : _invTransform(inverse(transform))
    {}
    
    ConstTransformer(float3 translation)
    : _invTransform(matrix4x4_translation(-translation))
    {}
    
    float3 transform(float3 p) const
    {
        const auto t = _invTransform * float4 { p.x, p.y, p.z, 1.f };
        return t.xyz;
    }
    
private:
    float4x4 _invTransform;
};
