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

class SDFTorus final
{
public:
    
    static ObjectType objectType() { return ObjectType::torus; }
    
    SDFTorus(float radius, float thickness)
    : _radiusThicknessAndPadding { radius, thickness, 0.f, 0.f }
    {}
    
    float radius() const { return _radiusThicknessAndPadding.x; }
    float thickness() const { return _radiusThicknessAndPadding.y; }
    
    float computeDistance(float3 p) const
    {
        float2 q { length(p.xz) - radius(), p.y };
        return length(q) - thickness();
    }
    
    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        const float t = thickness();
        const float amplitude = radius() + t;
        const float3 halfSize { amplitude, t, amplitude };
        
        return evaluateBoxCulling(halfSize, ray, outlineThickness);
    }
    
private:
    // _innerOuterRadiusAndPadding.x = inner
    // _innerOuterRadiusAndPadding.y = outer
    float4 _radiusThicknessAndPadding;
};

/*
template <>
float3 computeNormal<SDFSphere>(Sphere sphere, Ray ray, float dist, float3 position)
{
    return normalize(position - sphere.origin());
}
*/


