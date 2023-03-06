//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

class SDFGeometry final
{
public:
    
    float computeDistance(float3 p) const;
    bool evaluateCulling(Ray ray) const;
    
private:
    SDFGeometry() = delete;
};

template <typename TSDFGeometry>
float3 computeNormal(TSDFGeometry geometry, float dist, float3 position)
{
    constexpr float delta = 0.01f;
    float2 eps { delta, 0.f };
    
    return normalize(float3 {
        geometry.computeDistance(position + eps.xyy) - dist,
        geometry.computeDistance(position + eps.yxy) - dist,
        geometry.computeDistance(position + eps.yyx) - dist
    });
}

