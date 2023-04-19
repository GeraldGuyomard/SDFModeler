//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"
#include "ObjectHeadersArray.h"
#include "PrimitiveEvaluator.h"


INLINE float computeDistance(float3 pt,
                    const THREAD ObjectHeadersArray& headersArray,
                    THREAD size_t& objectIndex)
{
    DistanceEvaluator distanceEvaluator { pt };
    
    CONSTANT ObjectHeader* header = headersArray.header(objectIndex);
    const auto objectID = header->objectId;
    float2 distances { 1e7f, 1e7f };
    
    const size_t nbObjects = headersArray.nbObjects();
    
    while ((objectIndex < nbObjects) && (header->objectId == objectID))
    {
        const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
        distances[header->operation] = min(distances[header->operation], dist);
        
        header = headersArray.header(++objectIndex);
    }
    
    return max(distances.x, -distances.y);
}
