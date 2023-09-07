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
    const auto rot = transpose(_invRotTransform);
    
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
    _invRotTransform = transpose(rot);
    
    _translation = m.columns[3].xyz;
}

void
RSTTransformer::setScale(float s)
{
    _scale = s;
}

void
RSTTransformer::setTranslation(float3 t)
{
    _translation = t;
}

RSTTransformer::RSTTransformer(float4x4 transform)
{
    setTransform(transform);
}

inline float coef(const float3x3& m, size_t x, size_t y)
{
    return m.columns[y][x];
}

float3
RSTTransformer::rotationEulers() const
{
    // https://learnopencv.com/rotation-matrix-to-euler-angles/
    const auto R = transpose(_invRotTransform);
    const float sy = sqrtf(coef(R,0,0) * coef(R,0,0) +  coef(R,1,0) * coef(R,1,0) );
 
    const bool singular = sy < 1e-6; // If
 
    float x, y, z;
    if (!singular)
    {
        x = atan2f(coef(R,2,1) , coef(R,2,2));
        y = atan2f(-coef(R,2,0), sy);
        z = atan2f(coef(R,1,0), coef(R,0,0));
    }
    else
    {
        x = atan2f(-coef(R,1,2), coef(R,1,1));
        y = atan2f(-coef(R,2,0), sy);
        z = 0;
    }
    
    const float radToDeg = 180.f / M_PI;
    return float3 { x * radToDeg, y * radToDeg, z * radToDeg };
}

void
RSTTransformer::setRotationEulers(float3 xyz)
{
    const float degToRad = M_PI / 180.f;
    
    const float x = xyz.x * degToRad;
    const auto rotX = matrix3x3_rotation(x, float3 { 1, 0, 0});
    
    const float y = xyz.y * degToRad;
    const auto rotY = matrix3x3_rotation(y, float3 { 0, 1, 0});
    
    const float z = xyz.z * degToRad;
    const auto rotZ = matrix3x3_rotation(z, float3 { 0, 0, 1});
    
    const auto rot = rotZ * rotY * rotX;
    
    _invRotTransform = transpose(rot);
}
