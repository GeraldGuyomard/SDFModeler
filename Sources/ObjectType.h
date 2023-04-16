//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

enum class ObjectType : int64_t
{
    invalid = -1,
    
    sphere = 0,
    box,
    roundedBox,
    plane,
    grid,
    
    composition = 10
};

using ObjectID = uint32_t;
static CONSTANT constexpr ObjectID kInvalidObjectID = 0;

class RayMarchResult
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

class PickResult
{
public:
    ObjectID objectID = 0;
    float3 position = { 0 };
    
    PickResult() = default;
    
    PickResult(ObjectID objectID, float3 position)
    : objectID(objectID), position(position)
    {}
    
};
