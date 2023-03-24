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
        float3 q = abs(p) - halfSize();
        return length(max(q, 0.f)) + min(max(q.x, max(q.y, q.z)), 0.f) - radius();
    }
    
    bool evaluateCulling(Ray ray) const
    {
        const auto hSize = halfSize();
        const float r = max(max(hSize.x, hSize.y), hSize.z) * 2.f;
        
        return evaluateSphereCulling(r, ray);
    }
    
    float3 halfSize() const { return _halfSizeAndRadius.xyz; }
    float radius() const { return _halfSizeAndRadius.w; }
    
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
