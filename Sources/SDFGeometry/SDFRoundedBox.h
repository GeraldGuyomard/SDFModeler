//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"
#include "ObjectType.h"

class SDFRoundedBox final
{
public:
    
    static ObjectType objectType() { return ObjectType::roundedBox; }
    
    SDFRoundedBox(float3 halfSize, float radius)
    : _halfSizeAndRadius { halfSize.x, halfSize.y, halfSize.z, radius }
    {}
    
    float computeDistance(float3 p) const
    {
        float3 q = abs(p) - halfSize();
        return length(max(q, 0.f)) + min(max(q.x, max(q.y, q.z)), 0.f) - radius();
    }
    
    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        float3 h = halfSize();
        h += _halfSizeAndRadius.www;
        
        return evaluateBoxCulling(h, ray, outlineThickness);
    }
    
    BoundingBox boundingBox() const
    {
        float3 h = halfSize();
        h += _halfSizeAndRadius.www;
        
        return { -h, h };
    }
    
    float3 halfSize() const { return _halfSizeAndRadius.xyz; }
    float radius() const { return _halfSizeAndRadius.w; }
    
private:
    // xyz halfsize
    // w radius
    float4 _halfSizeAndRadius;
};
