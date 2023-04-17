//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "PrimitiveEvaluator.h"

struct SDFSerializedComposition final
{
    uint64_t nbObjectsToAdd;
    uint64_t nbObjectsToSubstract;
    
    MaterialID materialID;
    
    float extraCullingMargin = 0.f;
    
    // what should come next is
    // nbObjectsToAdd + nbObjectsToSubstract objects with ObjectHeaders
    
    SDFSerializedComposition(uint64_t nbObjectsToAdd,
                             uint64_t nbObjectsToSubstract,
                             MaterialID materialID,
                             float extraCullingMargin)
    : nbObjectsToAdd(nbObjectsToAdd),
    nbObjectsToSubstract(nbObjectsToSubstract),
    materialID(materialID),
    extraCullingMargin(extraCullingMargin)
    {}
    
    ObjectType objectType() const
    {
        return ObjectType::composition;
    }
};

class SDFComposition final
{
public:

    using Serialized = SDFSerializedComposition;
    
    static ObjectType objectType() { return ObjectType::composition; }
    
    SDFComposition(CONSTANT Serialized* serializedComposition) :
    _materialID(serializedComposition->materialID),
    _nbObjectsToAdd(serializedComposition->nbObjectsToAdd),
    _nbObjectsToSubstract(serializedComposition->nbObjectsToSubstract),
    _extraCullingMargin(serializedComposition->extraCullingMargin)
    {
        auto ptr = reinterpret_cast<CONSTANT uint8_t*>(serializedComposition);
        _firstHeader = reinterpret_cast<CONSTANT ObjectHeader*>(ptr + sizeof(Serialized));
    }
    
    bool evaluateCulling(Ray ray) const
    {
        CullEvaluatorWithExtraCullingMarginOverride cullEvaluator { ray, _extraCullingMargin };
        
        CONSTANT ObjectHeader* header = _firstHeader;
        
        for (uint64_t i=0; i < _nbObjectsToAdd; ++i)
        {
            const bool culled = evaluateAtomicPrimitive<CullEvaluatorWithExtraCullingMarginOverride, bool>(cullEvaluator, header);
            if (!culled)
            {
                return false;
            }
            
            header = ObjectHeader::next(header);
        }
        
        return true;
    }
    
    float computeDistance(float3 pt) const
    {
        DistanceEvaluator distanceEvaluator { pt };
        
        CONSTANT ObjectHeader* header = _firstHeader;
        
        uint64_t i = 0;
        float distanceOfAddition = 1e7f;
        for (; i < _nbObjectsToAdd; ++i)
        {
            const float d = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
            distanceOfAddition = min(distanceOfAddition, d);
            header = ObjectHeader::next(header);
        }
        
        const uint64_t n = _nbObjectsToAdd + _nbObjectsToSubstract;
        float distanceOfSubstraction = 1e7f;
        for (; i < n; ++i)
        {
            const float d = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
            distanceOfSubstraction = min(distanceOfSubstraction, d);
            header = ObjectHeader::next(header);
        }
        
        return max(distanceOfAddition, -distanceOfSubstraction);
    }
    
    MaterialID materialID() const { return _materialID; }
    
private:
    const MaterialID _materialID;
    
    const uint64_t _nbObjectsToAdd;
    const uint64_t _nbObjectsToSubstract;
    CONSTANT ObjectHeader* _firstHeader;
    
    float _extraCullingMargin = 0.f;
};

