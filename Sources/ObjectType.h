//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"

enum class ObjectType : int64_t
{
    invalid = -1,
    
    sphere = 0,
    box = 1,
    roundedBox = 2,
    plane = 3
};

template <typename TSDFGeometry>
ObjectType getObjectType()
{
    return ObjectType::invalid;
}

