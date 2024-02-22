//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "FragmentShader/FragmentShader.h"

class MattingShader final
{
public:
    
    MattingShader(CONSTANT Uniforms& uniforms, CONSTANT Materials& materials)
    {}
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        return float4 { 1.f, 1.f, 1.f, 1.f };
    }
};
