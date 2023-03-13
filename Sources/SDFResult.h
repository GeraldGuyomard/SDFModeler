//
//  SDFResult.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "SDFGeometry/SDFGeometry.h"
#include "Ray.h"

constexpr static CONSTANT float kDistanceEpsilon = 1e-3f;

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

template <typename TShader, typename TPrimitive>
SDFResult computeSDF(float3 p, Ray ray, TShader shader, TPrimitive primitive)
{
    if (primitive.culled())
    {
        return { 1e5f, 0.f };
    }
    
    const float d = primitive.computeDistance(p);
    
    if (d > kDistanceEpsilon)
    {
        return { d, 0.f };
    }
    
    const float4 c = shader.computeShade(primitive, ray, d, p);
    
    return { d, c };
}

template <typename TShader, typename TFirstPrimitive, typename... TPrimitives>
SDFResult computeSDF(float3 p, Ray ray, TShader shader, TFirstPrimitive firstPrimitive, TPrimitives... primitives)
{
    SDFResult r1 = computeSDF(p, ray, shader, firstPrimitive);
    SDFResult r2 = computeSDF(p, ray, shader, primitives...);
    
    return (r1.distance <= r2.distance) ? r1 : r2;
}
