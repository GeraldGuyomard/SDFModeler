//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "FragmentShader/FragmentShader.h"

class PhongShader final
{
public:
    
    PhongShader(CONSTANT Uniforms& uniforms, CONSTANT Materials& materials)
    : _lightDirection(normalize(uniforms.lightDirection)), _materials(materials)
    {}
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        const auto mat = _materials.materialByID(primitive.materialID());
        
        const float4 albedo = mat.computeAlbedo(ray, dist, p);
        const float3 normal = computeNormal(primitive, dist, p);
        
        float intensity = max(0.1f, dot(-normal, _lightDirection));

        // L = 2 * dot(N, L) * N - L
        float3 L = (2.f * dot(normal, _lightDirection) * normal) - _lightDirection;
        
        float spec = max(0.f, dot(ray.direction, L));
        spec = 0.8f * pow(spec, 30.f);
        
        return (albedo * float4 { intensity, intensity, intensity, 1.f } ) + float4 { spec, spec, spec, 0.f };
    }
    
    CONSTANT Materials& materials() const
    {
        return _materials;
    }
    
private:
    const float3 _lightDirection;
    CONSTANT Materials& _materials;
};
