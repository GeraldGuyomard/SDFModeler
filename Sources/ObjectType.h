//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

enum class ObjectType : int64_t
{
    invalid = -1,
    
    sphere = 0,
    box,
    roundedBox,
    plane,
    torus,
    cylinder
};

using ObjectID = uint16_t;
static CONSTANT constexpr ObjectID kInvalidObjectID = 0;

enum class SDFOperation : uint32_t
{
    addition = 0,
    substraction = 1
};
