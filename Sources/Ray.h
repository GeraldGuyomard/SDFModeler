//
//  Ray.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"

struct Ray final
{
    float3 origin;
    float3 direction;
    float maxLength;
    
    Ray(float3 origin, float3 direction, float maxLength)
    : origin(origin), direction(direction), maxLength(maxLength)
    {}
    
    float3 pt(float t) const
    {
        return origin + (t * direction);
    }
};

