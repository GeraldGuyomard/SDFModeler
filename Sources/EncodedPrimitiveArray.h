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

using TDrawCommandIndex = int16_t;
struct DrawCommand final // 4
{
    TPrimitiveOffset primitiveOffsetOrNegativeChildrenCount;
    int16_t ownerOffset; // 0 if self owned, negative otherwise
};
