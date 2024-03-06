//
//  RayMarchResult.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "SDFGeometry/SDFGeometry.h"
#include "Ray.h"
#include <TargetConditionals.h>

#if TARGET_OS_VISION
    constexpr static CONSTANT float kDistanceEpsilon = 0.5 * 1e-3f;
#else
    constexpr static CONSTANT float kDistanceEpsilon = 1e-3f;
#endif

class RayMarchResult final
{
public:
    const Ray ray;
    
    ObjectID objectID = 0;
    float4 color = { 0 };
    float distance = -1;
    
    RayMarchResult(Ray ray)
    : ray(ray)
    {}
    
    RayMarchResult(Ray ray, ObjectID objectID, float4 color,  float distance)
    : ray(ray), objectID(objectID), color(color), distance(distance)
    {}
    
    bool isValid() const
    {
        return distance >= 0;
    }
};

class PickResult final
{
public:
    ObjectID objectID = 0;
    float3 position = { 0 };
    
    PickResult() = default;
    
    PickResult(ObjectID objectID, float3 position)
    : objectID(objectID), position(position)
    {}
    
};


