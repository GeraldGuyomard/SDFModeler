//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"
#include "Uniforms.h"

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
    
    bool evaluateCulling(Ray ray) const
    {
        return evaluateSphereCulling(_radius, ray);
    }
    
    float radius() const { return _radius; }
    
private:
    float _radius;
};

template <>
INLINE ObjectType getObjectType<SDFSphere>()
{
    return ObjectType::sphere;
}

/*
template <>
float3 computeNormal<SDFSphere>(Sphere sphere, Ray ray, float dist, float3 position)
{
    return normalize(position - sphere.origin());
}
*/


