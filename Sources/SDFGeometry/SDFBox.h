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

class SDFBox final
{
public:
    
    SDFBox(float3 halfSize)
    : _halfSizeAndPadding { halfSize.x, halfSize.y, halfSize.z }
    {}
    
    float computeDistance(float3 p) const
    {
        float3 q = abs(p) - halfSize();
        return length(max(q, 0.f)) + min(max(q.x, max(q.y, q.z)), 0.f);
    }
    
    bool evaluateCulling(Ray ray) const
    {
        return evaluateBoxCulling(halfSize(), ray);
    }
    
    float3 halfSize() const { return _halfSizeAndPadding.xyz; }
    
private:
    // _halfSizeAndPadding.xyz = halfsize
    // _halfSizeAndPadding.w unused
    float4 _halfSizeAndPadding;
};

template <>
INLINE ObjectType getObjectType<SDFBox>()
{
    return ObjectType::box;
}
