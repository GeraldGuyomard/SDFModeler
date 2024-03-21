//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "FragmentShader/FragmentShader.h"

class FlatShader final
{
public:
    
    FlatShader(CONSTANT CameraUniforms& uniforms, CONSTANT Materials& materials)
    : _materials(materials)
    {}
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        const auto mat = _materials.materialByID(primitive.materialID());
        
        return mat.albedo();
    }
    
    CONSTANT Materials& materials() const
    {
        return _materials;
    }
    
private:
    CONSTANT Materials& _materials;
};
