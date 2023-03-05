//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"

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
    
private:
    const float3 _halfSize;
};
