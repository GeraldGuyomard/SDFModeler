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
    ShadedPrimitive(CONSTANT DrawCommand* cmd)
    : _cmd(cmd)
    {}
    
    MaterialID materialID() const
    {
        return 0; //_prim->materialId;
    }
    
    float computeDistance(float3 pt) const
    {
        return 1;
        //return ::computeDistance(pt, _prim);
    }
    
private:
    CONSTANT DrawCommand* _cmd;
};

