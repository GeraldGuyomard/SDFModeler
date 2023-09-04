//
//  SDFGeometry.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Transformer/StandardTransformers.h"
#include <math.h>

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

void
RSTTransformer::setScale(float s)
{
    _scale = s;
}

RSTTransformer::RSTTransformer(float4x4 transform)
{
    setTransform(transform);
}

inline float coef(const float3x3& m, size_t x, size_t y)
{
    return m.columns[y-1][x-1];
}

void
RSTTransformer::computeEulers(float& xAngle, float& yAngle, float& zAngle) const
{
    // http://eecs.qmul.ac.uk/~gslabaugh/publications/euler.pdf
    const auto rotMatrix = inverse(_invRotTransform);
    
    float& psi = xAngle;
    float& theta = yAngle;
    float& phi = zAngle;
    
    if ((coef(rotMatrix, 3, 1) != -1.f) && (coef(rotMatrix, 3, 1) != +1.f))
    {
        theta = -asinf(coef(rotMatrix, 3, 1));
        
        const float cosTheta = cosf(theta);
        psi = atan2f(coef(rotMatrix, 3, 2) / cosTheta, coef(rotMatrix, 3, 3) / cosTheta);
        phi = atan2f(coef(rotMatrix, 2, 1) / cosTheta, coef(rotMatrix, 1, 1) / cosTheta);
    }
    else
    {
        phi = 0.f;
        if (coef(rotMatrix, 3, 1) == -1.f)
        {
            theta = M_PI_2;
            psi = phi + atan2f(coef(rotMatrix, 1, 2), coef(rotMatrix, 1, 3));
        }
        else
        {
            theta = -M_PI_2;
            psi = -phi + atan2(-coef(rotMatrix, 1, 2), -coef(rotMatrix, 1, 3));
        }
    }
}
