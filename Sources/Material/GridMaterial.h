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
    
    float4 computeAlbedo(float3 p) const
    {
        float2 xy = p.xz;
        
        float2 s = sign(xy);
        xy *= s;
        
        xy = fmod(xy, _cellSize) / _cellSize;
        
        xy = max(-s, 0.f) + (s * xy);

        xy = step(0.9, xy);
        
        float l = length(xy);
        return _color * l;
    }
    
private:
    const float2 _cellSize;
    const float4 _color;
};
