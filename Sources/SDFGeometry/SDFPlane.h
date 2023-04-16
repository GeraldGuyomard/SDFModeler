//
//  SDFPlane.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"

INLINE float planeIntersection(Ray ray, float3 origin, float3 normal)
{
    // https://en.wikipedia.org/wiki/Line–plane_intersection
    // d = (p0 - l0) . n / (l . n)
    const float denum = dot(normal, ray.direction);
    const float d = dot((origin - ray.origin), normal) / denum;
    return d;
}

// horizontal plane at y = 0
class SDFPlane final
{
public:
    
    static GeometryType type() { return GeometryType::plane; }
    
    SDFPlane()
    {}
    
    float computeDistance(float3 p) const
    {
        return p.y;
    }
    
    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        const float d = raycast(ray);
        
        return (d < 0) || (d > ray.maxLength);
    }
    
    float raycast(Ray ray) const
    {
        return planeIntersection(ray, float3{0}, float3{ 0, 1, 0} );
    }
};
