//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"

class SDFRoundedBox final
{
public:
    
    SDFRoundedBox(float3 halfSize, float radius)
    : _halfSize(halfSize), _radius(radius)
    {}
    
    float computeDistance(float3 p) const
    {
        float3 q = abs(p) - _halfSize;
        return length(max(q, 0.f)) + min(max(q.x, max(q.y, q.z)), 0.f) - _radius;
    }
    
    bool evaluateCulling(Ray ray) const
    {
        const float r = max(max(_halfSize.x, _halfSize.y), _halfSize.z) * 2.f;
        
        return evaluateSphereCulling(r, ray);
    }
    
    float3 halfSize() const { return _halfSize; }
    
private:
    const float3 _halfSize;
    const float _radius;
};
