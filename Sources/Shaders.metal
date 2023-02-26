//
//  Shaders.metal
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

// File for Metal kernel and shader functions

#include <metal_stdlib>
#include <simd/simd.h>

// Including header shared between this Metal shader code and Swift/C code executing Metal API commands
#import "ShaderTypes.h"

using namespace metal;

struct VertexShaderOut
{
    float4 position [[position]];
    float2 viewportNDC;
};

constexpr static constant float kDistanceEpsilon = 1e-2f;

struct Ray
{
    float3 origin;
    float3 direction;
    float maxLength;
    
    Ray(float3 origin, float3 direction, float maxLength)
    : origin(origin), direction(direction), maxLength(maxLength)
    {}
    
    float3 pt(float t) const
    {
        return origin + (t * direction);
    }
};

vertex VertexShaderOut vertexShader(Vertex in [[stage_in]],
                               constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]])
{
    VertexShaderOut out;

    out.position = in.position;
    out.viewportNDC = in.viewportNDC;

    return out;
}

Ray computeRay(float2 ndcPosition, constant Uniforms& uniforms)
{
    float3 origin = viewToWorld(ndcPosition, 0, uniforms);
    float3 end = viewToWorld(ndcPosition, 1, uniforms);
    
    float3 direction = (end - origin);
    float maxDist = length(direction);
    direction /= maxDist;
    
    return { origin, direction, maxDist };
}

struct SDFResult
{
    float distance;
    float4 color;
    
    SDFResult()
    : distance(-10000), color(0.f)
    {}
    
    SDFResult(float distance, float4 color)
    : distance(distance), color(color)
    {}
    
    bool hit() const
    {
        return (distance >= 0.f) && (distance <= kDistanceEpsilon);
    }
    
    bool isValid() const
    {
        return (color.a != 0.f) && hit();
    }
};

template <typename TPrimitive>
float3 computeNormal(TPrimitive primitive, float dist, float3 position)
{
    constexpr float delta = 0.01f;
    float2 eps(delta, 0.f);
    
    return normalize(float3(primitive.computeDistance(position + eps.xyy) - dist,
             primitive.computeDistance(position + eps.yxy) - dist,
             primitive.computeDistance(position + eps.yyx) - dist));
}


template <typename TPrimitive>
float4 computeShade(TPrimitive primitive, Ray ray, float dist, float3 p)
{
    if (abs(dist) > kDistanceEpsilon)
    {
        return primitive.color(p);
    }
    
    float3 normal = computeNormal(primitive, dist, p);
    
    float3 lightDir = normalize(float3(-1, -1, -1));
    float intensity = max(0.1f, dot(-normal, lightDir));

    // L = 2 * dot(N, L) * N - L
    float3 L = (2.f * dot(normal, lightDir) * normal) - lightDir;
    
    float spec = max(0.f, dot(ray.direction, L));
    spec = 0.8f * pow(spec, 10.f);
    
    return (primitive.color(p) * intensity) + float4(spec, spec, spec, 0.f);
}


class Sphere
{
public:
    Sphere(float3 origin, float radius, float4 color)
    : _origin(origin), _radius(radius), _color(color)
    {}
    
    float computeDistance(float3 p) const
    {
        const float3 d = p - _origin;
        const float dist = length(d) - _radius;
        return dist;
    }
    
    float4 color(float3 p) const
    {
        return _color;
    }
    
    float3 origin() const
    {
        return _origin;
    }
    
private:
    const float3 _origin;
    const float _radius;
    const float4 _color;
};

/*
template <>
float3 computeNormal<Sphere>(Sphere sphere, Ray ray, float dist, float3 position)
{
    return normalize(position - sphere.origin());
}
*/

float rayPlaneIntersectionCoeff(Ray ray, float3 planeOrigin, float3 planeNormal)
{
    float denom = dot(planeNormal, ray.direction);
    if (abs(denom) > 0.0001f) // your favorite epsilon
    {
        float t = dot(planeOrigin - ray.origin, planeNormal) / denom;
        return t;
    }
    
    return -1.f;
}

class HorizontalPlane
{
public:
    
    HorizontalPlane(float altitude, float4 color)
    : _altitude(altitude), _color(color)
    {}
    
    float computeDistance(float3 p) const
    {
        return p.y - _altitude;
    }
    
    float4 color(float3 p) const
    {
        return _color;
    }
    
private:
    float _altitude;
    float4 _color;
};


float2 pulse(float2 v)
{
    return step(0.9, v);
}

class HorizontalGrid
{
public:
    
    HorizontalGrid(float altitude, float cellSize, float4 color)
    : _altitude(altitude), _cellSize(cellSize), _color(color)
    {}
    
    float computeDistance(float3 p) const
    {
        return p.y - _altitude;
    }
    
    float4 color(float3 p) const
    {
        float2 xy = fmod(abs(p.xz), _cellSize) / _cellSize;
        
        xy = pulse(xy);
        
        float l = length(xy);
        return _color * l;
    }
    
private:
    float _altitude;
    float _cellSize;
    float4 _color;
};

class RoundedBox
{
public:
    
    RoundedBox(float3 origin, float3 size, float radius, float4 color)
    : _origin(origin), _size(size), _radius(radius), _color(color)
    {}
    
    float computeDistance(float3 p) const
    {
        p -= _origin;
        
        float3 q = abs(p) - _size;
        return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - _radius;
    }
    
    float4 color(float3 p) const
    {
        return _color;
    }
    
private:
    const float3 _origin;
    const float3 _size;
    const float _radius;
    const float4 _color;
};

template<typename P1, typename P2>
class UnionPrimitive
{
public:
    UnionPrimitive(P1 p1, P2 p2, float4 color)
    : _p1(p1), _p2(p2), _color(color)
    {}
    
    float computeDistance(float3 p) const
    {
        const float d1 = _p1.computeDistance(p);
        const float d2 = _p2.computeDistance(p);
        return min(d1, d2);
    }
    
    float4 color(float3 p) const
    {
        return _color;
    }
    
private:
    P1 _p1;
    P2 _p2;
    float4 _color;
};

template<typename P1, typename P2>
class SubstractionPrimitive
{
public:
    SubstractionPrimitive(P1 p1, P2 p2, float4 color)
    : _p1(p1), _p2(p2), _color(color)
    {}
    
    float computeDistance(float3 p) const
    {
        const float d1 = _p1.computeDistance(p);
        const float d2 = _p2.computeDistance(p);
        return max(d1, -d2);
    }
    
    float4 color(float3 p) const
    {
        return _color;
    }
    
private:
    P1 _p1;
    P2 _p2;
    float4 _color;
};


template <typename TPrimitive>
SDFResult computeSDF(float3 p, Ray ray, TPrimitive primitive)
{
    const float d = primitive.computeDistance(p);
    const float4 c = computeShade(primitive, ray, d, p);
    
    return { d, c };
}

template <typename TFirstPrimitive, typename... TPrimitives>
SDFResult computeSDF(float3 p, Ray ray, TFirstPrimitive firstPrimitive, TPrimitives... primitives)
{
    SDFResult r1 = computeSDF(p, ray, firstPrimitive);
    SDFResult r2 = computeSDF(p, ray, primitives...);
    
    return (r1.distance <= r2.distance) ? r1 : r2;
}

template <typename... TPrimitives>
SDFResult rayMarch(Ray ray, TPrimitives... primitives)
{
    constexpr int kNbSteps = 100;
    
    float d = 0.f;
    
    for (int i=0; i < kNbSteps; ++i)
    {
        float3 p = ray.pt(d);
        auto result = computeSDF(p, ray, primitives...);
        
        if (result.hit())
        {
            return result;
        }
        
        d += result.distance;
        
        if (d > ray.maxLength)
        {
            break;
        }
    }
    
    return {};
}

class Scene
{
public:
    
    SDFResult rayMarch(Ray ray) const
    {
        //constexpr float kZ = -5.f;
        constexpr float kZ = 0;
        
        Sphere sphere1 { float3(-1, 0, kZ), 0.5f, float4(1, 0, 0, 1) };
        Sphere sphere2 { float3(0.5, 0, kZ), 0.8f, float4(0, 0, 1, 1) };
        Sphere sphere3 { float3(0, 1, kZ), 0.7f, float4(0, 1, 0, 1) };
        RoundedBox box { float3(0.5, 0, kZ + 1.5f), float3(0.2, 0.4, 0.2), 0.1, float4(1, 1, 1, 1) };
        
        Sphere sphere4 { float3(-1.5, 0.6, kZ + 2.5f), 0.4f, float4(0, 0, 1, 1) };
        RoundedBox box2 { float3(-1.5, 0, kZ + 2.5f), float3(0.2, 0.4, 0.2), 0.1, float4(1, 1, 1, 1) };
        UnionPrimitive<Sphere, RoundedBox> unionPrim(sphere4, box2, float4(0, 1, 1, 1));
        
        return ::rayMarch(ray, sphere1, sphere2, sphere3, box, unionPrim);
    }
};

class Environment
{
public:
    SDFResult rayMarch(Ray ray) const
    {
        constexpr float kGridGreyLevel = 0.5f;
        HorizontalGrid grid(-10, 1, float4(kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, 1));
        
        return ::rayMarch(ray, grid);
    }
};

fragment float4 fragmentShader(VertexShaderOut in [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(BufferIndexUniforms) ]])
{
    const auto ray = computeRay(in.viewportNDC, uniforms);
    
    Scene scene;
    auto res = scene.rayMarch(ray);
    if (res.isValid())
    {
        return res.color;
    }

    // Test Env Last
    Environment env;
    res = env.rayMarch(ray);
    if (res.isValid())
    {
        return res.color;
    }
        
    // Render background gradient
    // in.viewportNDC.y in [1, -1]
    float grey = (1.f - in.viewportNDC.y) / 2.f;
    return float4(grey, grey, grey, 1);
}
