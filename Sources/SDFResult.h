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

template <typename TPrimitive>
float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p)
{
    if (dist > kDistanceEpsilon)
    {
        return { 0.f };
    }
    
    float3 normal = computeNormal(primitive, dist, p);
    
    float3 lightDir = normalize(float3 { -1.f, -1.f, -1.f });
    float intensity = max(0.1f, dot(-normal, lightDir));

    // L = 2 * dot(N, L) * N - L
    float3 L = (2.f * dot(normal, lightDir) * normal) - lightDir;
    
    float spec = max(0.f, dot(ray.direction, L));
    spec = 0.8f * pow(spec, 30.f);
    
    return (primitive.computeAlbedo(p) * intensity) + float4 { spec, spec, spec, 0.f };
}

template <typename TPrimitive>
SDFResult computeSDF(float3 p, Ray ray, TPrimitive primitive)
{
    const float d = primitive.computeDistance(p);
    const float4 c = computeShade(primitive, ray, d, p);
    
    return { d, c };
}

template <typename TFirstPrimitive, typename... TPrimitives>
SDFResult computeSDF(float3 p, Ray ray, TFirstPrimitive firstPrimitive, TPrimitives... primitives)
{
    SDFResult r1 = computeSDF(p, ray, firstPrimitive);
    SDFResult r2 = computeSDF(p, ray, primitives...);
    
    return (r1.distance <= r2.distance) ? r1 : r2;
}
