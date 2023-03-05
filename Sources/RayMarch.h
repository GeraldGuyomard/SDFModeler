//
//  RayMarch.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"
#include "SDFGeometry/SDFGeometry.h"
#include "SDFResult.h"

template <typename TShader, typename TPrimitive>
SDFResult rayMarch(Ray ray, TShader shader, TPrimitive primitive)
{
    constexpr int kNbSteps = 100;
    
    float d = 0.f;
    
    for (int i=0; i < kNbSteps; ++i)
    {
        float3 p = ray.pt(d);
        auto result = computeSDF(p, ray, shader, primitive);
        
        if (result.hit())
        {
            return result;
        }
        
        d += result.distance;
        
        if (d > ray.maxLength)
        {
            break;
        }
    }
    
    return {};
}


template <typename TShader, typename... TPrimitives>
SDFResult rayMarch(Ray ray, TShader shader, TPrimitives... primitives)
{
    setCulling(ray, primitives...);
    
    constexpr int kNbSteps = 100;
    
    float d = 0.f;
    
    for (int i=0; i < kNbSteps; ++i)
    {
        float3 p = ray.pt(d);
        auto result = computeSDF(p, ray, shader, primitives...);
        
        if (result.hit())
        {
            return result;
        }
        
        d += result.distance;
        
        if (d > ray.maxLength)
        {
            break;
        }
    }
    
    return {};
}
