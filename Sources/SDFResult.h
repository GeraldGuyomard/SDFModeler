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
    ObjectID objectID = 0;
    
    SDFResult()
    : distance(-10000), color(0.f)
    {}
    
    SDFResult(ObjectID id, float distance, float4 color)
    : distance(distance), color(color), objectID(id)
    {}
    
    SDFResult(float distance, float4 color)
    : distance(distance), color(color)
    {}
};

