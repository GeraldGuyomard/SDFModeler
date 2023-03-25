//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "ObjectHeader.h"

class CompositeUnion final
{
public:
    
    CompositeUnion(CONSTANT ObjectHeader* header1, CONSTANT ObjectHeader* header2)
    : _header1(header1), _header2(header2)
    {}
    
    bool evaluateCulling(Ray ray) const
    {
        return false;
    }
    
    float computeDistance(float3 pt) const
    {
        return 0.f;
    }
    
    float4 computeAlbedo(float3 pt) const
    {
        return { 0 };
    }
    
private:
    CONSTANT ObjectHeader* _header1;
    CONSTANT ObjectHeader* _header2;
};

template <>
INLINE ObjectType getObjectType<CompositeUnion>()
{
    return ObjectType::compositeUnion;
}

