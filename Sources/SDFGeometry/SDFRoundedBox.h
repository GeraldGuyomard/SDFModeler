//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"

class SDFRoundedBox final
{
public:
    
    SDFRoundedBox(float3 halfSize, float radius)
    : _halfSizeAndRadius { halfSize.x, halfSize.y, halfSize.z, radius }
    {}
    
    float computeDistance(float3 p) const
    {
        float3 q = abs(p) - _halfSizeAndRadius.xyz;
        return length(max(q, 0.f)) + min(max(q.x, max(q.y, q.z)), 0.f) - _halfSizeAndRadius.w;
    }
    
    bool evaluateCulling(Ray ray) const
    {
        const float r = max(max(_halfSizeAndRadius.x, _halfSizeAndRadius.y), _halfSizeAndRadius.z) * 2.f;
        
        return evaluateSphereCulling(r, ray);
    }
    
    float3 halfSize() const { return _halfSizeAndRadius.xyz; }
    
private:
    // xyz halfsize
    // w radius
    float4 _halfSizeAndRadius;
};

template <>
INLINE ObjectType getObjectType<SDFRoundedBox>()
{
    return ObjectType::roundedBox;
}
