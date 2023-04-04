//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "Material/Material.h"

class SimpleMaterial final
{
public:
    
    SimpleMaterial() = default;
    
    SimpleMaterial(float4 albedo)
    : _albedo(albedo)
    {}
    
    float4 albedo() const { return _albedo; }
    
private:
    float4 _albedo = { 0, 0, 0, 0 };
};

struct Materials final
{
    size_t nbMaterials = 0;
    SimpleMaterial material[128];
    
    SimpleMaterial materialByID(MaterialID id) CONSTANT
    {
        return material[id];
    }
};
