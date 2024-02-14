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

using TPrimitiveOffset = uint16_t;
constexpr static CONSTANT TPrimitiveOffset kInvalidPrimitiveOffset = TPrimitiveOffset(-1);

class EncodedPrimitiveArray final
{
public:
    
    EncodedPrimitiveArray(CONSTANT uint8_t* buffer)
    : _buffer(buffer)
    {}
    
    size_t nbPrimitives() const { return _nbPrimitives; }
    
    CONSTANT EncodedPrimitive* primitive(size_t index) const
    {
        const size_t offset = _encodedPrimitiveOffset[index];
        CONSTANT uint8_t* ptr = _buffer + offset;
        return reinterpret_cast<CONSTANT EncodedPrimitive*>(ptr);
    }
    
    void add(CONSTANT EncodedPrimitive* prim)
    {
        const size_t offset = reinterpret_cast<CONSTANT uint8_t*>(prim) - _buffer;
        _encodedPrimitiveOffset[_nbPrimitives++] = TOffset(offset);
    }
    
    float computeDistance(float3 pt, THREAD size_t& index) const
    {
        DistanceEvaluator distanceEvaluator { pt };
        
        CONSTANT EncodedPrimitive* prim = primitive(index);
        const auto objectID = prim->objectId;
        float2 distances { 1e7f, 1e7f };
        
        const size_t nbPrims = nbPrimitives();
        
        while ((index < nbPrims) && (prim->objectId == objectID))
        {
            const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
            distances[prim->operation] = min(distances[prim->operation], dist);
            
            prim = primitive(++index);
        }
        
        return max(distances.x, -distances.y);
    }
    
private:
    CONSTANT uint8_t* _buffer;
    size_t _nbPrimitives = 0;
    
    using TOffset = uint16_t;
    TPrimitiveOffset _encodedPrimitiveOffset[kNbObjectsMax];
};

