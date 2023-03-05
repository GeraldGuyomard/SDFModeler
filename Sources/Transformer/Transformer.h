//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

class Transformer final
{
public:
    
    template <typename TSDFGeometry>
    float computeSDF(TSDFGeometry primitive, float3 p) const;
    
    Ray localRay(Ray ray) const;
    
private:
    Transformer() = delete;
};

