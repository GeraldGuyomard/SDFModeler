//
//  Utilities.h
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include <simd/simd.h>

#if defined(__METAL_VERSION__)

    #include <metal_stdlib>

    using namespace metal;

    #define CONSTANT constant
    #define INLINE

    using EnumBackingType = metal::int32_t;
    #define VB_ATTRIBUTE(a) [[attribute(a)]]


#else

    using namespace simd;

    #define CONSTANT const
    #define INLINE inline

    using EnumBackingType = int32_t;
    #define VB_ATTRIBUTE(a)

#endif

INLINE float4x4 matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ)
{
    float ys = 1 / tan(fovyRadians * 0.5);
    float xs = ys / aspect;
    float zs = farZ / (nearZ - farZ);

    float4x4 m;
    m.columns[0] = { xs,   0,          0,  0 };
    m.columns[1] = {  0,  ys,          0,  0 };
    m.columns[2] = {  0,   0,         zs, -1 };
    m.columns[3] = {  0,   0, nearZ * zs,  0 };
    
    return m;
}

INLINE float4x4 matrix4x4_translation(float3 t)
{
    float4x4 m;
    
    m.columns[0] = { 1,   0,  0,  0 };
    m.columns[1] = { 0,   1,  0,  0 };
    m.columns[2] = { 0,   0,  1,  0 };
    m.columns[3] = { t.x, t.y, t.z,  1 };
    
    return m;
}

INLINE float4x4 matrix4x4_rotation(float radians, float3 axis)
{
    axis = normalize(axis);
    float ct = cos(radians);
    float st = sin(radians);
    float ci = 1 - ct;
    float x = axis.x, y = axis.y, z = axis.z;

    return (simd_float4x4) {{
        { ct + x * x * ci,     y * x * ci + z * st, z * x * ci - y * st, 0},
        { x * y * ci - z * st,     ct + y * y * ci, z * y * ci + x * st, 0},
        { x * z * ci + y * st, y * z * ci - x * st,     ct + z * z * ci, 0},
        {                   0,                   0,                   0, 1}
    }};
}

INLINE float4x4 matrix4x4_rotation(float radians, float3 axis, float3 origin)
{
    const auto rot = matrix4x4_rotation(radians, axis);
    
    return matrix4x4_translation(origin) * rot * matrix4x4_translation(-origin);
}

