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
INLINE float4 render(float2 viewportNDC, TShader shader, CONSTANT Uniforms& uniforms, CONSTANT DynamicScene& dynamicScene)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    DynamicObject<TShader> dynObject { shader, dynamicScene };
    const auto res = dynObject.rayMarch(ray);
    
    if (res.isValid())
    {
        return res.color;
    }
    
    // Test Env Last
    Environment env;
    const auto envRes = env.rayMarch(ray, shader);

    return envRes.color;
}

