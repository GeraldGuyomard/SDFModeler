//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "PrimitiveEvaluator.h"

enum class CompositionOperation : uint64_t
{
    addition,
    substraction
};

template <typename TTransformer, typename TMaterial>
struct SDFSerializedComposition final
{
    CompositionOperation operation;
    uint64_t padding = 0;
    
    TTransformer transformer;
    TMaterial material;
    
    // what should come next is
    // 2 ObjectHeaders
    
    SDFSerializedComposition(CompositionOperation op, TTransformer transformer, TMaterial material)
    : operation(op), transformer(transformer), material(material)
    {}
};

template <typename TTransformer, typename TMaterial>
class SDFComposition final
{
public:
    
    using Serialized = SDFSerializedComposition<TTransformer, TMaterial>;
    
    SDFComposition(CONSTANT Serialized* serializedComposition)
    : _operation(serializedComposition->operation),
    _transformer(serializedComposition->transformer),
    _material(serializedComposition->material)
    {
        auto ptr = reinterpret_cast<CONSTANT uint8_t*>(serializedComposition);
        auto headersStart = ptr + sizeof(Serialized);
        
        _header1 = reinterpret_cast<CONSTANT ObjectHeader*>(headersStart);
        _header2 = reinterpret_cast<CONSTANT ObjectHeader*>(headersStart + _header1->byteSize);
    }
    
    bool evaluateCulling(Ray ray) const
    {
        CullEvaluator cullEvaluator { ray };
        
        switch (_operation)
        {
            case CompositionOperation::addition:
            {
                return evaluateAtomicPrimitive<CullEvaluator, bool>(cullEvaluator, _header1)
                && evaluateAtomicPrimitive<CullEvaluator, bool>(cullEvaluator, _header2);
            }
                
            case CompositionOperation::substraction:
            {
                return evaluateAtomicPrimitive<CullEvaluator, bool>(cullEvaluator, _header1);
            }
        }
        
        return false;
    }
    
    float computeDistance(float3 pt) const
    {
        DistanceEvaluator distanceEvaluator { pt };
        const float d1 = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, _header1);
        const float d2 = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, _header2);
        
        switch (_operation)
        {
            case CompositionOperation::addition:
            {
                return min(d1, d2);
            }
                
            case CompositionOperation::substraction:
            {
                return max(d1, -d2);
            }
        }
        
        return 0.f;
    }
    
    float4 computeAlbedo(float3 pt) const
    {
        return _material.computeAlbedo(pt);
    }
    
private:
    const CompositionOperation _operation;
    const TTransformer _transformer;
    const TMaterial _material;
    
    CONSTANT ObjectHeader* _header1;
    CONSTANT ObjectHeader* _header2;
};


