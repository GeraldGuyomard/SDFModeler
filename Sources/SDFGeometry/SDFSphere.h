//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"
#include "ObjectType.h"

class SDFSphere final
{
public:
    
    SDFSphere(float radius)
    : _radiusAndPadding(radius)
    {}
    
    float computeDistance(float3 p) const
    {
        return length(p) - radius();
    }
    
    bool evaluateCulling(Ray ray) const
    {
        return evaluateSphereCulling(radius(), ray);
    }
    
    float radius() const { return _radiusAndPadding.x; }
    
private:
    float4 _radiusAndPadding;
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


