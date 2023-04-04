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
    
    float4 computeAlbedo(Ray ray, float dist, float3 p) const
    {
        return _albedo;
    }
    
private:
    float4 _albedo = { 0, 0, 0, 0 };
};

struct Materials final
{
    size_t nbMaterials = 0;
    ConstMaterial material[128];
    
    ConstMaterial materialByID(MaterialID id) CONSTANT
    {
        return material[id];
    }
};
