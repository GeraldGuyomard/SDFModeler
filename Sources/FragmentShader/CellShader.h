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
    
    CellShader(float3 lightDirection)
    : _lightDirection(normalize(lightDirection))
    {}
    
    template <typename TPrimitive>
    float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p) const
    {
        const float4 albedo = primitive.computeAlbedo(p);
        const float3 normal = computeNormal(primitive, dist, p);
        
        float intensity = max(0.1f, dot(-normal, _lightDirection));
        
        float3 color = (albedo.xyz * float3 { intensity, intensity, intensity } );
        
        color = round(color * 3.f) / 3.f;
        
        // silhouette
        const float s = step(0.2f, abs(dot(-normal, ray.direction)));
        color *= s;
        
        return float4 { color.r, color.g, color.b, 1.f };
    }
    
private:
    const float3 _lightDirection;
};
