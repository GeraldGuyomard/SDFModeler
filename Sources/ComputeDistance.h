//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "EncodedPrimitive.h"
#include "EncodedPrimitiveArray.h"
#include "PrimitiveEvaluator.h"


INLINE float computeDistance(float3 pt,
                    const THREAD EncodedPrimitiveArray& encodedPrimitivesArray,
                    THREAD size_t& index)
{
    DistanceEvaluator distanceEvaluator { pt };
    
    CONSTANT EncodedPrimitive* prim = encodedPrimitivesArray.primitive(index);
    const auto objectID = prim->objectId;
    float2 distances { 1e7f, 1e7f };
    
    const size_t nbPrims = encodedPrimitivesArray.nbPrimitives();
    
    while ((index < nbPrims) && (prim->objectId == objectID))
    {
        const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
        distances[prim->operation] = min(distances[prim->operation], dist);
        
        prim = encodedPrimitivesArray.primitive(++index);
    }
    
    return max(distances.x, -distances.y);
}
