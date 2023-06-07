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
    
    static ObjectType objectType() { return ObjectType::sphere; }
    
    SDFSphere(float radius)
    : _radiusAndPadding(radius)
    {}
    
    float computeDistance(float3 p) const
    {
        return length(p) - radius();
    }
    
    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        return evaluateSphereCulling(radius() + outlineThickness, ray);
    }
    
    BoundingBox boundingBox() const
    {
        const float r = radius();
        const float3 halfSize { r, r, r };
        
        return { -halfSize, halfSize };
    }
    
    float radius() const { return _radiusAndPadding.x; }
    void setRadius(float radius);
    
private:
    float4 _radiusAndPadding;
};

/*
template <>
float3 computeNormal<SDFSphere>(Sphere sphere, Ray ray, float dist, float3 position)
{
    return normalize(position - sphere.origin());
}
*/


