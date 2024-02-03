//
//  CommonDefinitions.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "CommonDefinitions.h"

#include <cmath>
#include <assert.h>

bool isValid(const float4x4& m)
{
    for (size_t i=0; i < 4; ++i)
    {
        for (size_t j=0; j < 4; ++j)
        {
            assert(!std::isnan(m.columns[i][j]));
        }
    }
    
    return true;
}
