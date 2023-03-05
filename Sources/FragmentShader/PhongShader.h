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
    
    PhongShader() = default;
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        const float4 albedo = primitive.computeAlbedo(p);
        const float3 normal = computeNormal(primitive, dist, p);
        
        float3 lightDir = normalize(float3 { -1.f, -1.f, -1.f });
        float intensity = max(0.1f, dot(-normal, lightDir));

        // L = 2 * dot(N, L) * N - L
        float3 L = (2.f * dot(normal, lightDir) * normal) - lightDir;
        
        float spec = max(0.f, dot(ray.direction, L));
        spec = 0.8f * pow(spec, 30.f);
        
        return (albedo * float4 { intensity, intensity, intensity, 1.f } ) + float4 { spec, spec, spec, 0.f };
    }
    
private:
};
