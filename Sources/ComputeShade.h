//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"
#include "PrimitiveEvaluator.h"
#include "ComputeDistance.h"

class ShadedPrimitive final
{
public:
    ShadedPrimitive(const THREAD ObjectHeadersArray& headersArray, size_t objectIndex)
    : _headersArray(headersArray), _objectIndex(objectIndex)
    {}
    
    MaterialID materialID() const
    {
        auto header = _headersArray.headers[_objectIndex];
        return header->materialId;
    }
    
    float computeDistance(float3 pt) const
    {
        auto index = _objectIndex;
        return ::computeDistance(pt, _headersArray, index);
    }
    
private:
    const THREAD ObjectHeadersArray& _headersArray;
    const size_t _objectIndex;
};

template <typename TShader>
inline float4 computeShade(TShader shader,
                           Ray ray,
                           float dist,
                           float3 pt,
                           const THREAD ObjectHeadersArray& headersArray,
                           size_t objectIndex)
{
    ShadedPrimitive primitive { headersArray, objectIndex };
    return shader.computeShade(primitive, ray, dist, pt);
}
