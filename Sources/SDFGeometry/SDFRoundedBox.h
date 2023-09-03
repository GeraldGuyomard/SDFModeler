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
    
    // for editing
    float width() const { return _halfSizeAndRadius.x * 2; }
    void setWidth(float w) { _halfSizeAndRadius.x = w * 0.5f; }
    
    float height() const { return _halfSizeAndRadius.y * 2; }
    void setHeight(float h) { _halfSizeAndRadius.y = h * 0.5f; }
    
    float depth() const { return _halfSizeAndRadius.z * 2; }
    void setDepth(float d) { _halfSizeAndRadius.z = d * 0.5f; }
    
    void setRadius(float radius) { _halfSizeAndRadius.w = radius; }
    
private:
    // xyz halfsize
    // w radius
    float4 _halfSizeAndRadius;
};
