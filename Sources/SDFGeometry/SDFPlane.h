//
//  SDFPlane.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "ObjectType.h"

class SDFPlane final
{
public:
    
    SDFPlane() = default;
    
    float computeDistance(float3 p) const
    {
        return p.y;
    }
    
    bool evaluateCulling(Ray ray) const
    {
        // https://en.wikipedia.org/wiki/Line–plane_intersection
        // d = (p0 - l0) . n / (l . n)
        // p0 = (0, 0, 0), n = (0, 1, 0)
        // -> -l0. y / l.y
        const float d = -ray.origin.y / ray.direction.y;
        
        return (d < 0) || (d > ray.maxLength);
    }
};

template <>
INLINE ObjectType getObjectType<SDFPlane>()
{
    return ObjectType::plane;
}
