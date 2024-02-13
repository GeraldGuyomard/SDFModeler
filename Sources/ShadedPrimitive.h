//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "EncodedPrimitive.h"
#include "PrimitiveEvaluator.h"
#include "ComputeDistance.h"

class ShadedPrimitive final
{
public:
    ShadedPrimitive(const THREAD EncodedPrimitiveArray& primArray, size_t index)
    : _primsArray(primArray), _primIndex(index)
    {}
    
    MaterialID materialID() const
    {
        auto prim = _primsArray.primitive(_primIndex);
        return prim->materialId;
    }
    
    float computeDistance(float3 pt) const
    {
        auto index = _primIndex;
        return ::computeDistance(pt, _primsArray, index);
    }
    
private:
    const THREAD EncodedPrimitiveArray& _primsArray;
    const size_t _primIndex;
};

