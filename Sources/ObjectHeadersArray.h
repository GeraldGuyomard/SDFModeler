//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "ObjectHeader.h"

constexpr static CONSTANT size_t kNbObjectsMax = 128;

struct ObjectHeadersArray final
{
    CONSTANT ObjectHeader* headers[kNbObjectsMax];
    size_t nbObjects = 0;
};

