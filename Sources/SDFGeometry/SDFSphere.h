//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"

class SDFSphere final
{
public:
    
    SDFSphere(float radius)
    : _radius(radius)
    {}
    
    float computeDistance(float3 p) const
    {
        return length(p) - _radius;
    }
    
    float radius() const { return _radius; }
    
private:
    const float _radius;
};

/*
template <>
float3 computeNormal<SDFSphere>(Sphere sphere, Ray ray, float dist, float3 position)
{
    return normalize(position - sphere.origin());
}
*/

template <>
INLINE bool evaluateCulling<SDFSphere>(SDFSphere sphere, Ray ray)
{
    // https://en.wikipedia.org/wiki/Line–sphere_intersection
    // (ray.dir . (ray.origin - sphere.origin))^2 - ((ray.origin - sphere.origin)ˆ2 - (sphere.radius ^ 2))
    // sphere.origin = 0, 0, 0
    //
    // => (ray.dir . ray.origin)^2 - (ray.origin ˆ 2 - (sphere.radius ^ 2))
    const float a = dot(ray.direction, ray.origin);
    const float r = sphere.radius();
    const float d = (a * a) - (dot(ray.origin, ray.origin) - (r * r));
    
    return d < 0.f;
}

