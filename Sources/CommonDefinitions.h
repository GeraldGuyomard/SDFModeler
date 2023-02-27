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
    #define DEVICE device
    #define INLINE

    using EnumBackingType = metal::int32_t;
    #define VB_ATTRIBUTE(a) [[attribute(a)]]

    float4x4 float4x4_identity()
    {
        return float4x4 {   {1, 0, 0, 0},
                            {0, 1, 0, 0},
                            {0, 0, 1, 0},
                            {0, 0, 0, 1}
        };
    }

#else

    using namespace simd;

    #define CONSTANT const
    #define DEVICE
    #define INLINE inline

    using EnumBackingType = int32_t;
    #define VB_ATTRIBUTE(a)

    inline float4x4 float4x4_identity()
    {
        return matrix_identity_float4x4;
    }

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

INLINE float3 translation(float4x4 m)
{
    return m.columns[3].xyz;
}

INLINE float3 right(float4x4 m)
{
    return normalize(m.columns[0].xyz);
}

INLINE float3 up(float4x4 m)
{
    return normalize(m.columns[1].xyz);
}

INLINE float3 forward(float4x4 m)
{
    return normalize(m.columns[2].xyz);
}

INLINE void setTranslation(DEVICE float4x4& m, float3 t)
{
    m.columns[3].xyz = t;
}

struct float4x4Decomposition final
{
    float3 right;
    float3 up;
    float3 forward;
    float3 position;
};

INLINE float4x4Decomposition decompose(float4x4 m)
{
    float4x4Decomposition decomp;
    
    decomp.right = normalize(m.columns[0].xyz);
    decomp.up = normalize(m.columns[1].xyz);
    decomp.forward = normalize(m.columns[2].xyz);
    decomp.position = m.columns[3].xyz;
    
    return decomp;
}

INLINE float4x4 recompose(float4x4Decomposition decomp)
{
    auto m = float4x4_identity();
    
    m.columns[0].xyz = decomp.right;
    m.columns[1].xyz = decomp.up;
    m.columns[2].xyz = decomp.forward;
    m.columns[3].xyz = decomp.position;
    
    return m;
}
