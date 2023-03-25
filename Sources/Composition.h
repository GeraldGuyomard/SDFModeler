//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "ObjectHeader.h"

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
        return false;
    }
    
    float computeDistance(float3 pt) const
    {
        //ComputeDistanceEvaluator ev;
        
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


