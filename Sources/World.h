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
#include "DynamicSDFObject.h"

#include "FragmentShader/PhongShader.h"
#include "FragmentShader/CellShader.h"

class Scene
{
public:
    
    template <typename TShader>
    SDFResult rayMarch(Ray ray, TShader shader)
    {
        constexpr float kZ = 0;
        
        Sphere redSphere { ray, { 0.5f }, { float3 { -1, 0, kZ } }, { float4 { 1, 0, 0, 1 } } };
        
        float3 pos = float3 {0.5, 0, kZ};
        
        constexpr float s = 0.5f;
        
        RoundedBox whiteBox
        {
            ray,
            { float3 { 0.4f, 0.6f, 0.4f }, 0.1f }, // geometry
            { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s }, // transform
            { float4 { 1, 1, 1, 1 } } // material
        };
        
        Box whiteBoxHalf
        {
            ray,
            { float3 { 0.4f * s, 0.6f * s, 0.4f * s } }, // geometry
            { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) }, // transform
            { float4 { 1, 1, 1, 1 } } // material
        };
        
        Sphere blueSphere { ray, { 0.4f }, { pos }, { float4 { 0, 0, 1, 1 } } };
        Sphere greenSphere { ray, { 0.7f }, { float3 { 0, 1, kZ } }, { float4 { 0, 1, 0, 1 } } };
        
        Sphere spherePart { ray, { 0.4f }, // geom
            { float3 { -2., 0.6, kZ + 0.5f } } // transform
        };
        
        RoundedBox boxPart { ray,
            { float3 { 0.2, 0.4, 0.2 }, 0.1 }, // geometry
            { float3 { -2., 0, kZ + 0.5f } } // transform
        };
        
        Sphere negativeSpherePart { ray, { 0.4f }, // geom
            { float3 { -2., 0.3, kZ + 1.f } } // transform
        };
        
        using TUnion = SDFUnion<Sphere, RoundedBox>;
        TUnion sdfUnion(spherePart, boxPart);
        
        using TComposite = SDFSubstraction<TUnion, Sphere>;
        TComposite composite(sdfUnion, negativeSpherePart);
        
        SDFObject<TComposite, RSTTransformer, ConstMaterial> uni(ray, composite, {}, { float4 { 0, 1, 1, 1 } } );
        
        return ::rayMarch(ray, shader, redSphere, blueSphere, greenSphere, whiteBox, whiteBoxHalf, uni);
    }
};

class Environment
{
public:
    
    template <typename TShader>
    SDFResult rayMarch(Ray ray, TShader shader)
    {
        constexpr float kGridGreyLevel = 0.5f;
        const float4 color{ kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, 1 };
        //Plane grid({}, { float3(-10.f) }, { color } );
        
        Grid grid(ray, {}, { float3(-10.f) }, { 1.f , color });
        
        const auto res = ::rayMarch(ray, shader, grid);
        
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

template <typename TShader>
INLINE float4 render(float2 viewportNDC, TShader shader, CONSTANT Uniforms& uniforms, CONSTANT DynamicScene& mutableState)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    DynamicObject<TShader> dynObject { mutableState };
    auto res = dynObject.rayMarch(ray, shader);
    if (res.isValid())
    {
        return res.color;
    }
    
    Scene scene;
    res = scene.rayMarch(ray, shader);
    if (res.isValid())
    {
        return res.color;
    }

    // Test Env Last
    Environment env;
    res = env.rayMarch(ray, shader);

    return res.color;
}

