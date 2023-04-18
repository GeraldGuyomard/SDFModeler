//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"
#include "PrimitiveEvaluator.h"

struct ComputeDistanceResult
{
    const float distance;
    const size_t objectIndex;
    
    ComputeDistanceResult(float distance, size_t objectIndex)
    : distance(distance), objectIndex(objectIndex)
    {}
};

static inline ComputeDistanceResult computeDistance(float3 pt, CONSTANT ObjectHeader* headers[], size_t objectIndex, size_t nbObjects)
{
    DistanceEvaluator distanceEvaluator { pt };
    
    CONSTANT ObjectHeader* startHeader = headers[objectIndex];
    const auto objectID = startHeader->objectId;
    float2 distances { 1e7f, 1e7f };
    
    CONSTANT ObjectHeader* h = startHeader;
    
    while ((objectIndex < nbObjects) && (h->objectId == objectID))
    {
        const float dist = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, h);
        distances[h->operation] = min(distances[h->operation], dist);
        
        h = headers[++objectIndex];
    }
    
    const float dist = max(distances.x, -distances.y);
    return { dist, objectIndex };
}
