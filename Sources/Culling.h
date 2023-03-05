//
//  Culling.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Ray.h"
#include "Transformer/Transformer.h"

template <typename TSDFGeometry>
INLINE bool evaluateCulling(TSDFGeometry geometry, Ray ray)
{
    return false;
}
