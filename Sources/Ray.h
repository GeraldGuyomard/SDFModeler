//
//  Ray.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Uniforms.h"

struct Ray final
{
    float3 origin;
    float3 direction;
    float maxLength;
    
    Ray() = default;
    
    Ray(float3 origin, float3 direction, float maxLength)
    : origin(origin), direction(direction), maxLength(maxLength)
    {}
    
    float3 pt(float t) const
    {
        return origin + (t * direction);
    }
    
    bool containsPoint(float t) const
    {
        return (t >= 0) && (t <= maxLength);
    }
    
    static Ray make(float2 ndcPosition, CONSTANT Uniforms& uniforms)
    {
        float3 origin = viewToWorld(ndcPosition, uniforms.nearZInNDC, uniforms.cameraMatrix, uniforms.invProjectionMatrix);
        float3 end = viewToWorld(ndcPosition, uniforms.farZInNDC, uniforms.cameraMatrix, uniforms.invProjectionMatrix);
        
        float3 direction = (end - origin);
        direction = normalize(direction);
        
        Ray ray { origin, direction, uniforms.rayLength };
        
        return ray;
    }
};

