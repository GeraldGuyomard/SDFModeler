//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"
#include "ObjectType.h"

class SDFGeometry final
{
public:
    
    float computeDistance(float3 p) const;
    bool evaluateCulling(Ray ray, float outlineThickness) const;
    
private:
    SDFGeometry() = delete;
};

template <typename TSDFGeometry>
float3 computeNormal(TSDFGeometry geometry, float dist, float3 position)
{
#if 1
    
    constexpr float h = 0.0001; // replace by an appropriate value
    const float2 k { h, -h };
    return normalize(   k.xyy * geometry.computeDistance( position + k.xyy ) +
                        k.yyx * geometry.computeDistance( position + k.yyx ) +
                        k.yxy * geometry.computeDistance( position + k.yxy ) +
                        k.xxx * geometry.computeDistance( position + k.xxx ) );
    
#else
    constexpr float delta = 0.01f;
    float2 eps { delta, 0.f };
    
    return normalize(float3 {
        geometry.computeDistance(position + eps.xyy) - dist,
        geometry.computeDistance(position + eps.yxy) - dist,
        geometry.computeDistance(position + eps.yyx) - dist
    });
    
#endif
}

