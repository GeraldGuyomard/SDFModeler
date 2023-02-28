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

#include "SDFObject.h"
#include "Transformer/ConstTransformer.h"
#include "Material/ConstMaterial.h"
#include "Material/GridMaterial.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"

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

using Sphere = SDFObject<SDFSphere, ConstTransformer, ConstMaterial>;


using Plane = SDFObject<SDFPlane, ConstTransformer, ConstMaterial>;
using Grid = SDFObject<SDFPlane, ConstTransformer, GridMaterial>;
using RoundedBox = SDFObject<SDFRoundedBox, ConstTransformer, ConstMaterial>;

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
    
    float4 computeAlbedo(float3 p) const
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
        
        Sphere sphere1 { { 0.5f }, { float3(-1, 0, kZ) }, { float4(1, 0, 0, 1) } };
        Sphere sphere2 { { 0.8f }, { float3(0.5, 0, kZ) }, { float4(0, 0, 1, 1) } };
        Sphere sphere3 { { 0.7f }, { float3(0, 1, kZ) }, { float4(0, 1, 0, 1) } };
        RoundedBox box {
            { float3(0.2f, 0.4f, 0.2f), 0.1 }, // geometry
            { float3(0.5, 0, kZ + 1.5f) }, // transform
            { float4(1, 1, 1, 1) } // material
        };
        
        Sphere sphere4 { { 0.4f }, // geom
                         { float3(-1.5, 0.6, kZ + 2.5f) } // material
        };
        
        RoundedBox box2 {
            { float3(0.2, 0.4, 0.2), 0.1 }, // geometry
            { float3(-1.5, 0, kZ + 2.5f) } // transform
        };
        
        using TUnion = SDFUnion<Sphere, RoundedBox>;
        TUnion sdfUnion(sphere4, box2);
        
        SDFObject<TUnion, ConstTransformer, ConstMaterial> uni(sdfUnion, {}, { float4(0, 1, 1, 1) } );
        
        return ::rayMarch(ray, sphere1, sphere2, sphere3, box, uni);
    }
};

class Environment
{
public:
    SDFResult rayMarch(Ray ray) const
    {
        constexpr float kGridGreyLevel = 0.5f;
        const float4 color{ kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, 1 };
        //Plane grid({}, { float3(-10.f) }, { color } );
        
        Grid grid({}, { float3(-10.f) }, { 1.f , color });
        
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
