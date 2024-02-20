//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "EncodedPrimitive.h"
#include "Ray.h"
#include "PrimitiveEvaluator.h"

constexpr static CONSTANT size_t kNbObjectsMax = 64;

using TPrimitiveOffset = int16_t;
constexpr static CONSTANT TPrimitiveOffset kInvalidPrimitiveOffset = -1;

INLINE float computeDistance(float3 pt, CONSTANT EncodedPrimitive* prim)
{
    DistanceEvaluator distanceEvaluator { pt };
    
    float2 distances { 1e7f, 1e7f };
    
    const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
    distances[prim->operation] = min(distances[prim->operation], dist);
    
    return max(distances.x, -distances.y);
}

using TDrawCommandIndex = int16_t;
struct DrawCommand final
{
    uint16_t depth = 0;
    TPrimitiveOffset primitiveOffsetOrNegativeChildrenCount = 0; // -1 if no primitive
    ObjectID objectID;
};
