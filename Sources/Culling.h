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
