//
//  SDFPlane.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"

class SDFPlane final
{
public:
    
    SDFPlane() = default;
    
    float computeDistance(float3 p) const
    {
        return p.y;
    }
    
    bool evaluateCulling(Ray ray) const
    {
        return false;
    }
};
