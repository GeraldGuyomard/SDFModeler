//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Results.h"
#include "Material/SimpleMaterial.h"

class FragmentShader final
{
public:
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const;
    
    CONSTANT Materials& materials() const;
    
private:
    FragmentShader() = delete;
};

class NoShader final
{
public:
    NoShader(CONSTANT Uniforms&, CONSTANT Materials&) {}
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        return { 1, 1, 1, 1 };
    }
};
