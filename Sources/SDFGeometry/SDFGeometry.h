//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

enum class GeometryType : int64_t
{
    invalid = -1,
    
    sphere = 0,
    box,
    roundedBox,
    plane,
    grid
};


class SDFGeometry final
{
public:
    
    static GeometryType type();
    
    float computeDistance(float3 p) const;
    bool evaluateCulling(Ray ray, float outlineThickness) const;
    
private:
    SDFGeometry() = delete;
};

template <typename TSDFGeometry>
float3 computeNormal(TSDFGeometry geometry, float dist, float3 position)
{
    constexpr float h = 0.0001; // replace by an appropriate value
    const float2 k { h, -h };
    return normalize(   k.xyy * geometry.computeDistance( position + k.xyy ) +
                        k.yyx * geometry.computeDistance( position + k.yyx ) +
                        k.yxy * geometry.computeDistance( position + k.yxy ) +
                        k.xxx * geometry.computeDistance( position + k.xxx ) );
    
}

