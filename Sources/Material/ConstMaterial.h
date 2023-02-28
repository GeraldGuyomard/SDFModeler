//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "Material/Material.h"

class ConstMaterial final
{
public:
    
    ConstMaterial() = default;
    
    ConstMaterial(float4 albedo)
    : _albedo(albedo)
    {}
    
    float4 computeAlbedo(float3 p) const
    {
        return _albedo;
    }
    
private:
    const float4 _albedo = { 0 };
};
