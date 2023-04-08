//
//  SDFPlane.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "ObjectType.h"

INLINE float planeIntersection(Ray ray, float3 origin, float3 normal)
{
    // https://en.wikipedia.org/wiki/Line–plane_intersection
    // d = (p0 - l0) . n / (l . n)
    const float denum = dot(normal, ray.direction);
    const float d = dot((origin - ray.origin), normal) / denum;
    return d;
}

class SDFPlane final
{
public:
    
    static ObjectType objectType() { return ObjectType::plane; }
    
    SDFPlane(float3 origin = {0}, float3 normal = { 0, 1, 0} )
    : _origin(origin), _normal(normal)
    {}
    
    float computeDistance(float3 p) const
    {
        return (p - _origin).y;
    }
    
    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        const float d = raycast(ray);
        
        return (d < 0) || (d > ray.maxLength);
    }
    
    float raycast(Ray ray) const
    {
        return planeIntersection(ray, _origin, _normal );
    }
    
private:
    float3 _origin;
    float3 _normal;
};
