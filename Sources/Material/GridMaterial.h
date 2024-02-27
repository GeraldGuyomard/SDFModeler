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
        const float4 primaryColor = computeAlbedoNearest(ray, dist);
        return primaryColor;
        /*
        const float4 c0 = computeAlbedoNearest(ray.computeNeighbourRay({-1.f, 0.f}), dist);
        const float4 c1 = computeAlbedoNearest(ray.computeNeighbourRay({0.f, 1.f}), dist);
        const float4 c2 = computeAlbedoNearest(ray.computeNeighbourRay({1.f, 1.f}), dist);
        const float4 c3 = computeAlbedoNearest(ray.computeNeighbourRay({-1.f, 1.f}), dist);
        
        constexpr float kPrimaryWeight = 1.f;
        return ((primaryColor * kPrimaryWeight) + c0 + c1 + c2 + c3) / (4 + kPrimaryWeight);*/
    }
    
private:
    
    float4 computeAlbedoNearest(Ray ray, float dist) const
    {
        const float3 p = ray.pt(dist);
        
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
        
        //c = mix(c, fogColor, fogRatio);
        
        return c;
    }
    
    const float2 _cellSize;
    const float4 _color;
};
