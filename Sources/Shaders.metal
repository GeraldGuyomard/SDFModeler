//
//  Shaders.metal
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

// File for Metal kernel and shader functions


#include "ShaderTypes.h"
#include "Ray.h"
#include "SDFResult.h"
#include "RayMarch.h"

#include "SDFGeometry/SDFSphere.h"
#include "Transformer/ConstTransformer.h"

struct VertexShaderOut
{
    float4 position [[position]];
    float2 viewportNDC;
};

vertex VertexShaderOut vertexShader(Vertex in [[stage_in]],
                               constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]])
{
    VertexShaderOut out;

    out.position = in.position;
    out.viewportNDC = in.viewportNDC;

    return out;
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
        
        xy = step(0.9, xy);
        
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
    const auto ray = Ray::make(in.viewportNDC, uniforms);
    
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
