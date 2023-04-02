//
//  GridMaterial.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "Material/Material.h"

class GridMaterial final
{
public:
    
    GridMaterial(float2 cellSize, float4 color)
    : _cellSize(cellSize), _color(color)
    {}
    
    float4 computeAlbedo(Ray ray, float dist, float3 p) const
    {
        const float3 eps { 0.001f * dist, 0, 0 };
        
        const float4 c0 = computeAlbedoNearest(p);
        const float4 c1 = computeAlbedoNearest(p + eps.xyy);
        const float4 c2 = computeAlbedoNearest(p + eps.xyx);
        const float4 c3 = computeAlbedoNearest(p + eps.yyx);
        
        const float4 c01 = mix(c0, c1, 0.5f);
        const float4 c23 = mix(c2, c3, 0.5f);
        
        return mix(c01, c23, 0.5f);
    }
    
private:
    
    float4 computeAlbedoNearest(float3 p) const
    {
        float2 xy = p.xz;
        
        float2 s = sign(xy);
        xy *= s;
        
        xy = fmod(xy, _cellSize) / _cellSize;
        
        xy = max(-s, 0.f) + (s * xy);

        xy = step(0.9, xy);
        
        float l = length(xy);
        float4 c = _color * l;
        c = min(float4 {1, 1, 1, 1}, c);
        
        return c;
    }
    
    const float2 _cellSize;
    const float4 _color;
};
