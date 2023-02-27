//
//  SDFResult.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"

constexpr static CONSTANT float kDistanceEpsilon = 1e-2f;

struct SDFResult final
{
    float distance;
    float4 color;
    
    SDFResult()
    : distance(-10000), color(0.f)
    {}
    
    SDFResult(float distance, float4 color)
    : distance(distance), color(color)
    {}
    
    bool hit() const
    {
        return (distance >= 0.f) && (distance <= kDistanceEpsilon);
    }
    
    bool isValid() const
    {
        return (color.a != 0.f) && hit();
    }
};
