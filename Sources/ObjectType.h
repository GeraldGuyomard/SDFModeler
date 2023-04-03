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
    box,
    roundedBox,
    plane,
    grid,
    
    composition = 10
};

using ObjectID = uint32_t;
