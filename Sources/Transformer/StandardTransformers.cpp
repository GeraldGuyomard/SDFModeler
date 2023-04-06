//
//  SDFGeometry.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Transformer/StandardTransformers.h"

float4x4
RSTTransformer::transform() const
{
    const auto rot = inverse(_invRotTransform);
    
    float4x4 m;
    for (size_t i=0; i < 3; ++i)
    {
        m.columns[i] = float4 { rot.columns[i].x, rot.columns[i].y, rot.columns[i].z, 0.f } * _scale;
    }
    
    m.columns[3] = float4 { _translation.x, _translation.y, _translation.z, 1.f };
    
    return m;
}

void
RSTTransformer::setTransform(float4x4 m)
{
    float3 x = m.columns[0].xyz;
    _scale = length(x);
    
    float3x3 rot;
    for (size_t i=0; i < 3; ++i)
    {
        const float3 v = m.columns[i].xyz / _scale;
        rot.columns[i] = v;
    }
    _invRotTransform = inverse(rot);
    
    _translation = m.columns[3].xyz;
}
