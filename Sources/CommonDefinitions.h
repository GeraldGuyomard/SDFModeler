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

    inline float3 min(float3 lhs, float rhs)
    {
        return min(lhs, float3 { rhs });
    }

    inline float3 max(float3 lhs, float rhs)
    {
        return max(lhs, float3 { rhs });
    }

    inline float2 fmod(float2 in, float m)
    {
        return fmod(in, float2 { m });
    }

    inline float2 step(float m, float2 in)
    {
        return step(in, float2 { m });
    }

#endif

INLINE float4x4 float4x4_inverse(float4x4 m)
{
    float det = determinant(m);
    
    float A2323 = m.columns[2][2] * m.columns[3][3] - m.columns[2][3] * m.columns[3][2];
    float A1323 = m.columns[2][1] * m.columns[3][3] - m.columns[2][3] * m.columns[3][1];
    float A1223 = m.columns[2][1] * m.columns[3][2] - m.columns[2][2] * m.columns[3][1];
    float A0323 = m.columns[2][0] * m.columns[3][3] - m.columns[2][3] * m.columns[3][0];
    float A0223 = m.columns[2][0] * m.columns[3][2] - m.columns[2][2] * m.columns[3][0];
    float A0123 = m.columns[2][0] * m.columns[3][1] - m.columns[2][1] * m.columns[3][0];
    float A2313 = m.columns[1][2] * m.columns[3][3] - m.columns[1][3] * m.columns[3][2];
    float A1313 = m.columns[1][1] * m.columns[3][3] - m.columns[1][3] * m.columns[3][1];
    float A1213 = m.columns[1][1] * m.columns[3][2] - m.columns[1][2] * m.columns[3][1];
    float A2312 = m.columns[1][2] * m.columns[2][3] - m.columns[1][3] * m.columns[2][2];
    float A1312 = m.columns[1][1] * m.columns[2][3] - m.columns[1][3] * m.columns[2][1];
    float A1212 = m.columns[1][1] * m.columns[2][2] - m.columns[1][2] * m.columns[2][1];
    float A0313 = m.columns[1][0] * m.columns[3][3] - m.columns[1][3] * m.columns[3][0];
    float A0213 = m.columns[1][0] * m.columns[3][2] - m.columns[1][2] * m.columns[3][0];
    float A0312 = m.columns[1][0] * m.columns[2][3] - m.columns[1][3] * m.columns[2][0];
    float A0212 = m.columns[1][0] * m.columns[2][2] - m.columns[1][2] * m.columns[2][0];
    float A0113 = m.columns[1][0] * m.columns[3][1] - m.columns[1][1] * m.columns[3][0];
    float A0112 = m.columns[1][0] * m.columns[2][1] - m.columns[1][1] * m.columns[2][0];

    return (1.f / det) * float4x4 {
        float4 { + ( m.columns[1][1] * A2323 - m.columns[1][2] * A1323 + m.columns[1][3] * A1223 ),
            - ( m.columns[0][1] * A2323 - m.columns[0][2] * A1323 + m.columns[0][3] * A1223 ),
            + ( m.columns[0][1] * A2313 - m.columns[0][2] * A1313 + m.columns[0][3] * A1213 ),
            - ( m.columns[0][1] * A2312 - m.columns[0][2] * A1312 + m.columns[0][3] * A1212 ) },
        float4 {
            - ( m.columns[1][0] * A2323 - m.columns[1][2] * A0323 + m.columns[1][3] * A0223 ),
            + ( m.columns[0][0] * A2323 - m.columns[0][2] * A0323 + m.columns[0][3] * A0223 ),
            - ( m.columns[0][0] * A2313 - m.columns[0][2] * A0313 + m.columns[0][3] * A0213 ),
            + ( m.columns[0][0] * A2312 - m.columns[0][2] * A0312 + m.columns[0][3] * A0212 ), },
        float4 {
            + ( m.columns[1][0] * A1323 - m.columns[1][1] * A0323 + m.columns[1][3] * A0123 ),
            - ( m.columns[0][0] * A1323 - m.columns[0][1] * A0323 + m.columns[0][3] * A0123 ),
            + ( m.columns[0][0] * A1313 - m.columns[0][1] * A0313 + m.columns[0][3] * A0113 ),
            - ( m.columns[0][0] * A1312 - m.columns[0][1] * A0312 + m.columns[0][3] * A0112 ), },
        
        float4 {
            - ( m.columns[1][0] * A1223 - m.columns[1][1] * A0223 + m.columns[1][2] * A0123 ),
            + ( m.columns[0][0] * A1223 - m.columns[0][1] * A0223 + m.columns[0][2] * A0123 ),
            - ( m.columns[0][0] * A1213 - m.columns[0][1] * A0213 + m.columns[0][2] * A0113 ),
            + ( m.columns[0][0] * A1212 - m.columns[0][1] * A0212 + m.columns[0][2] * A0112 )}
    };
}

#if defined(__METAL_VERSION__)

float4x4 inverse(const float4x4 m)
{
    return float4x4_inverse(m);
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

INLINE float4x4 matrix4x4_scale(float3 s)
{
    float4x4 m;
    
    m.columns[0] = { s.x, 0,  0,  0 };
    m.columns[1] = { 0,   s.y,  0,  0 };
    m.columns[2] = { 0,   0,  s.z,  0 };
    m.columns[3] = { 0,   0,  0,  1 };
    
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
