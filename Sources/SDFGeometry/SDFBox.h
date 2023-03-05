//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"

class SDFBox final
{
public:
    
    SDFBox(float3 halfSize)
    : _halfSize(halfSize)
    {}
    
    float computeDistance(float3 p) const
    {
        float3 q = abs(p) - _halfSize;
        return length(max(q, 0.f)) + min(max(q.x, max(q.y, q.z)), 0.f);
    }
    
    float3 halfSize() const { return _halfSize; }
    
private:
    const float3 _halfSize;
};

template <>
INLINE bool evaluateCulling<SDFBox>(SDFBox box, Ray ray)
{
    const float3 size = box.halfSize();
    
    const float r = max(max(size.x, size.y), size.z) * 2.f;
    
    return evaluateSphereCulling(r, ray);
}
