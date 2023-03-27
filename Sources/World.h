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
#include "Scene.h"

#include "FragmentShader/PhongShader.h"
#include "FragmentShader/CellShader.h"

class Environment
{
public:
    
    template <typename TShader>
    SDFResult rayMarch(Ray ray, TShader shader)
    {
        constexpr float kGridGreyLevel = 0.5f;
        const float4 color{ kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, 1 };
        //Plane grid({}, { float3(-10.f) }, { color } );
        
        Grid grid({}, { float3(-10.f) }, { 1.f , color });
        
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
INLINE float4 render(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedScene& serializedScene)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    const TShader shader { uniforms.lightDirection };
    
    Scene<TShader> dynObject { shader, serializedScene };
    const auto res = dynObject.rayMarch(ray);
    
    if (res.isValid())
    {
        return res.color;
    }
    
    // cos angle
    const float grey = abs(ray.direction.y);
    const float4 c = { grey, grey, grey, 1.f };
    
    return c;
    /*
    // Test Env Last
    Environment env;
    const auto envRes = env.rayMarch(ray, shader);

    return envRes.color;*/
}

INLINE float4 renderPhong(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedScene& serializedScene)
{
    return render<PhongShader>(viewportNDC, uniforms, serializedScene);
}

INLINE float4 renderCellShaded(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedScene& serializedScene)
{
    return render<CellShader>(viewportNDC, uniforms, serializedScene);
}

INLINE float4 renderDefault(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedScene& serializedScene)
{
    return renderPhong(viewportNDC, uniforms, serializedScene);
}
