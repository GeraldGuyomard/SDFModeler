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

using TPrimitiveOffset = int16_t;
constexpr static CONSTANT TPrimitiveOffset kInvalidPrimitiveOffset = -1;

enum DrawCommandFlags : uint16_t
{
    fIsPrimitive = 1 << 0,
    fIsAdditivePrimitive = fIsPrimitive | (1 << 1),
    fIsSubtractivePrimitive = fIsPrimitive | (1 << 2)
};

using TDrawCommandIndex = int16_t;
struct DrawCommand final // 4
{
    uint16_t flags; // 2
    
    union
    {
        TPrimitiveOffset primitiveOffset;
        uint16_t childrenCount;
    };
    
    int16_t ownerOffset; // 0 if self owned, negative otherwise
    
    bool isPrimitive() CONSTANT
    {
        return (flags & fIsPrimitive) != 0;
    }
    
    bool isAdditivePrimitive() CONSTANT
    {
        return (flags & fIsAdditivePrimitive) == fIsAdditivePrimitive;
    }
    
    bool isSubtractivePrimitive() CONSTANT
    {
        return (flags & fIsSubtractivePrimitive) == fIsSubtractivePrimitive;
    }
    
    bool isGroup() CONSTANT
    {
        return !isPrimitive();
    }
    
    
};
