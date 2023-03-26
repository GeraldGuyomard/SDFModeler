//
//  Culling.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Ray.h"
#include "Transformer/Transformer.h"

INLINE bool evaluateSphereCulling(float radius, Ray ray)
{
    // https://en.wikipedia.org/wiki/Line–sphere_intersection
    // (ray.dir . (ray.origin - sphere.origin))^2 - ((ray.origin - sphere.origin)ˆ2 - (sphere.radius ^ 2))
    // sphere.origin = 0, 0, 0
    //
    // => (ray.dir . ray.origin)^2 - (ray.origin ˆ 2 - (sphere.radius ^ 2))
    const float a = dot(ray.direction, ray.origin);
    const float d = (a * a) - (dot(ray.origin, ray.origin) - (radius * radius));
    
    return d < 0.f;
}

INLINE bool evaluateBoxCulling(float3 halfSize, Ray ray)
{
    const float3 boxMin = -halfSize;
    const float3 boxMax = halfSize;
    
    const float3 invDir = float3 { 1, 1, 1 } / ray.direction;
    
    float3 tMin = (boxMin - ray.origin) * invDir;
    float3 tMax = (boxMax - ray.origin) * invDir;
    float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    
    return tNear > tFar;
}
