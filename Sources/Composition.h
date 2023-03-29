//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "PrimitiveEvaluator.h"

template <typename TTransformer, typename TMaterial>
struct SDFSerializedComposition final
{
    uint64_t nbObjectsToAdd;
    uint64_t nbObjectsToSubstract;
    
    TTransformer transformer;
    TMaterial material;
    
    // what should come next is
    // nbObjectsToAdd + nbObjectsToSubstract objects with ObjectHeaders
    
    SDFSerializedComposition(uint64_t nbObjectsToAdd, uint64_t nbObjectsToSubstract, TTransformer transformer, TMaterial material)
    : nbObjectsToAdd(nbObjectsToAdd), nbObjectsToSubstract(nbObjectsToSubstract), transformer(transformer), material(material)
    {}
    
    ObjectType objectType() const
    {
        return ObjectType::composition;
    }
};

template <typename TTransformer, typename TMaterial>
class SDFComposition final
{
public:
    
    constexpr static CONSTANT size_t kNbObjectsMax = 16;
    
    using Serialized = SDFSerializedComposition<TTransformer, TMaterial>;
    using Transformer = TTransformer;
    using Material = TMaterial;
    
    static ObjectType objectType() { return ObjectType::composition; }
    
    SDFComposition(CONSTANT Serialized* serializedComposition) :
    _transformer(serializedComposition->transformer),
    _material(serializedComposition->material),
    _nbObjectsToAdd(serializedComposition->nbObjectsToAdd),
    _nbObjectsToSubstract(serializedComposition->nbObjectsToSubstract)
    {
        auto ptr = reinterpret_cast<CONSTANT uint8_t*>(serializedComposition);
        _firstHeader = reinterpret_cast<CONSTANT ObjectHeader*>(ptr + sizeof(Serialized));
    }
    
    bool evaluateCulling(Ray ray) const
    {
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT ObjectHeader* header = _firstHeader;
        
        for (uint64_t i=0; i < _nbObjectsToAdd; ++i)
        {
            const bool culled = evaluateAtomicPrimitive<CullEvaluator, bool>(cullEvaluator, header);
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
    
    float4 computeAlbedo(float3 pt) const
    {
        return _material.computeAlbedo(pt);
    }
    
private:
    const TTransformer _transformer;
    const TMaterial _material;
    
    uint64_t _nbObjectsToAdd;
    uint64_t _nbObjectsToSubstract;
    CONSTANT ObjectHeader* _firstHeader;
};

