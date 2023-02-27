//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"

class SDFGeometry final
{
public:
    
    float computeDistance(float3 p) const;
    
private:
    SDFGeometry() = delete;
};

template <typename TPrimitive>
float3 computeNormal(TPrimitive primitive, float dist, float3 position)
{
    constexpr float delta = 0.01f;
    float2 eps { delta, 0.f };
    
    return normalize(float3(primitive.computeDistance(position + eps.xyy) - dist,
             primitive.computeDistance(position + eps.yxy) - dist,
             primitive.computeDistance(position + eps.yyx) - dist));
}
