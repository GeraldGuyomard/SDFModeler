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

class SDFCylinder final
{
public:
    
    static ObjectType objectType() { return ObjectType::cylinder; }
    
    SDFCylinder(float radius, float height)
    : _radiusHeightAndPadding { radius, height, 0.f, 0.f }
    {}
    
    float radius() const { return _radiusHeightAndPadding.x; }
    float height() const { return _radiusHeightAndPadding.y; }
    
    float computeDistance(float3 p) const
    {
        const float2 d = abs(float2 { length(p.xz), p.y }) - _radiusHeightAndPadding.xy;
        return min(max(d.x,d.y), 0.f) + length(max(d,0.f));
    }
    
    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        return evaluateBoxCulling(halfSize(), ray, outlineThickness);
    }
    
    BoundingBox boundingBox() const
    {
        const float3 halfSize = this->halfSize();
        
        return { -halfSize, halfSize };
    }
    
    float3 halfSize() const
    {
        return { radius(), height(), radius() };
    }
    
    // For editing
    void setRadius(float);
    void setHeight(float);
    
private:
    // _innerOuterRadiusAndPadding.x = inner
    // _innerOuterRadiusAndPadding.y = outer
    float4 _radiusHeightAndPadding;
};



