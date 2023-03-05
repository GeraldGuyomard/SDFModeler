//
//  World.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/28/23.
//

#pragma once

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
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"

using Sphere = SDFObject<SDFSphere, ConstTransformer, ConstMaterial>;
using Plane = SDFObject<SDFPlane, ConstTransformer, ConstMaterial>;
using Grid = SDFObject<SDFPlane, ConstTransformer, GridMaterial>;
using Box = SDFObject<SDFBox, ConstTransformer, ConstMaterial>;
using RoundedBox = SDFObject<SDFRoundedBox, ConstTransformer, ConstMaterial>;

class Scene
{
public:
    
    SDFResult rayMarch(Ray ray) const
    {
        constexpr float kZ = 0;
        
        Sphere redSphere { { 0.5f }, { float3 { -1, 0, kZ } }, { float4 { 1, 0, 0, 1 } } };
        
        float3 pos = float3 {0.5, 0, kZ};
        
        constexpr float s = 0.5f;
        
        Box whiteBox
        {
            { float3 { 0.4f, 0.4f, 0.4f } }, // geometry
            { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s }, // transform
            { float4 { 1, 1, 1, 1 } } // material
        };
        
        Box whiteBoxHalf {
            { float3 { 0.4f * s, 0.4f * s, 0.4f * s } }, // geometry
            { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) }, // transform
            { float4 { 1, 1, 1, 1 } } // material
        };
        
        Sphere blueSphere { { 0.4f }, { pos }, { float4 { 0, 0, 1, 1 } } };
        Sphere greenSphere { { 0.7f }, { float3 { 0, 1, kZ } }, { float4 { 0, 1, 0, 1 } } };
        
        Sphere spherePart { { 0.4f }, // geom
            { float3 { -1.5, 0.6, kZ + 2.5f } } // material
        };
        
        RoundedBox boxPart {
            { float3 { 0.2, 0.4, 0.2 }, 0.1 }, // geometry
            { float3 { -1.5, 0, kZ + 2.5f } } // transform
        };
        
        using TUnion = SDFUnion<Sphere, RoundedBox>;
        TUnion sdfUnion(spherePart, boxPart);
        
        SDFObject<TUnion, ConstTransformer, ConstMaterial> uni(sdfUnion, {}, { float4 { 0, 1, 1, 1 } } );
        
        return ::rayMarch(ray, redSphere, blueSphere, greenSphere, whiteBox, whiteBoxHalf, uni);
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
        
        const auto res = ::rayMarch(ray, grid);
        
        if (res.isValid())
        {
            return res;
        }
        
        // cos angle
        const float grey = abs(ray.direction.y);
        const float4 c = { grey, grey, grey, 1.f };
        
        return { 0.f, c };
    }
};

INLINE float4 render(float2 viewportNDC, CONSTANT Uniforms& uniforms)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    Scene scene;
    auto res = scene.rayMarch(ray);
    if (res.isValid())
    {
        return res.color;
    }

    // Test Env Last
    Environment env;
    res = env.rayMarch(ray);

    return res.color;
}

