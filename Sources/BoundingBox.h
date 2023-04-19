//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

class BoundingBox final
{
public:
    float3 minPoint;
    float3 maxPoint;
    
    BoundingBox(float3 minPoint, float3 maxPoint)
    : minPoint(minPoint), maxPoint(maxPoint)
    {}
    
    bool empty() const
    {
        return (minPoint.x > maxPoint.x) || (minPoint.y > maxPoint.y) || (minPoint.z > maxPoint.z);
    }
};
