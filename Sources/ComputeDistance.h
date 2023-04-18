//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"
#include "PrimitiveEvaluator.h"


static float computeDistance(float3 pt,
                             const THREAD ObjectHeadersArray& headersArray,
                                   THREAD size_t& objectIndex)
{
    DistanceEvaluator distanceEvaluator { pt };
    
    CONSTANT ObjectHeader* header = headersArray.headers[objectIndex];
    const auto objectID = header->objectId;
    float2 distances { 1e7f, 1e7f };
    
    while ((objectIndex < headersArray.nbObjects) && (header->objectId == objectID))
    {
        const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
        distances[header->operation] = min(distances[header->operation], dist);
        
        header = headersArray.headers[++objectIndex];
    }
    
    return max(distances.x, -distances.y);
}
