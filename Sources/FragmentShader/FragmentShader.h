//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "SDFResult.h"

class FragmentShader final
{
public:
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const;
    
private:
    FragmentShader() = delete;
};
