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
    
    SimpleMaterial(float4 albedo, bool selected = false)
    : _albedo(albedo), _selected(false)
    {}
    
    float4 albedo() const { return _albedo; }
    
    bool selected() const { return _selected; }
    void setSelected(bool selected) { _selected = selected; }
    
private:
    float4 _albedo = { 0, 0, 0, 0 };
    bool _selected = false;
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
