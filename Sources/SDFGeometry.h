//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"

class SDFGeometry final
{
public:
    
    float computeDistance(float3 p) const;
    
private:
    SDFGeometry() = delete;
};
