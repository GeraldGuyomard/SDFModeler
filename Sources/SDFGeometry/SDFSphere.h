//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "SDFGeometry/SDFGeometry.h"

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
    
private:
    const float _radius;
};
