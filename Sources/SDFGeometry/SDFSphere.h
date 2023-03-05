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
    return evaluateSphereCulling(sphere.radius(), ray);
}

