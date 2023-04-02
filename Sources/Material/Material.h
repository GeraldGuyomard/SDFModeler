//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

class Material final
{
public:
    
    float4 computeAlbedo(Ray ray, float dist, float3 p) const;
    
private:
    Material() = delete;
};

