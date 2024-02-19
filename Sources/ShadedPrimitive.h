//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "EncodedPrimitive.h"
#include "PrimitiveEvaluator.h"
#include "EncodedPrimitiveArray.h"

class ShadedPrimitive final
{
public:
    ShadedPrimitive(CONSTANT EncodedPrimitive* prim)
    : _prim(prim)
    {}
    
    MaterialID materialID() const
    {
        return _prim->materialId;
    }
    
    float computeDistance(float3 pt) const
    {
        return ::computeDistance(pt, _prim);
    }
    
private:
    CONSTANT EncodedPrimitive* _prim;
};

