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
        float2 xy = p.xz;
        
        float2 s = sign(xy);
        xy *= s;
        
        xy = fract(xy / _cellSize);
        
        xy = max(-s, 0.f) + (s * xy);

        xy = step(0.98, xy);
        
        float pixelVisible = min(1.f, xy.x + xy.y);
        float4 c = _color * pixelVisible;
        
        // Fade to black (Fog)
        const float4 fogColor = { 0, 0, 0, 0};
        float fogRatio = dist / 10.f;
        fogRatio = pow(fogRatio, 1.5f);
        
        c = mix(c, fogColor, fogRatio);
        
        return c;
    }
    
private:
    
    const float2 _cellSize;
    const float4 _color;
};
