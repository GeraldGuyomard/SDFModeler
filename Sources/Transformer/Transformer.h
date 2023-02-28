//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"

class Transformer final
{
public:
    
    float3 transform(float3 p) const;
    
private:
    Transformer() = delete;
};

