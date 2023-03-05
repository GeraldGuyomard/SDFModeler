//
//  SDFRoundedBox.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "FragmentShader/FragmentShader.h"

class CellShader final
{
public:
    
    CellShader() = default;
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        const float4 albedo = primitive.computeAlbedo(p);
        const float3 normal = computeNormal(primitive, dist, p);
        
        float3 lightDir = normalize(float3 { -1.f, -1.f, -1.f });
        float intensity = max(0.1f, dot(-normal, lightDir));
        
        float3 color = (albedo.xyz * float3 { intensity, intensity, intensity } );
        
        color = round(color * 3.f) / 3.f;
        
        // silhouette
        const float s = step(0.2f, abs(dot(-normal, ray.direction)));
        color *= s;
        
        return float4 { color.r, color.g, color.b, 1.f };
    }
    
private:
};
