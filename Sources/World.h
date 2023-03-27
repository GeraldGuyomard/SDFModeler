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

template <typename TShader>
INLINE SDFResult rayMarchEnvironment(TShader shader, Ray ray, CONSTANT SerializedWorld& serializedWorld)
{
    Objects<TShader> env { shader, serializedWorld.environment };
    return env.rayMarch(ray);
}

template <typename TShader>
INLINE float4 render(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedWorld& serializedWorld)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    const TShader shader { uniforms.lightDirection };
    
    Objects<TShader> content { shader, serializedWorld.content };
    const auto contentRes = content.rayMarch(ray);
    
    if (contentRes.isValid())
    {
        return contentRes.color;
    }
    
    const auto envRes = rayMarchEnvironment(shader, ray, serializedWorld);
    if (envRes.isValid())
    {
        return envRes.color;
    }
    
    // Background
    const float grey = abs(ray.direction.y);
    const float4 c = { grey, grey, grey, 1.f };
    return c;
}

INLINE float4 renderPhong(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedWorld& serializedWorld)
{
    return render<PhongShader>(viewportNDC, uniforms, serializedWorld);
}

INLINE float4 renderCellShaded(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedWorld& serializedWorld)
{
    return render<CellShader>(viewportNDC, uniforms, serializedWorld);
}

INLINE float4 renderDefault(float2 viewportNDC, CONSTANT Uniforms& uniforms, CONSTANT SerializedWorld& serializedWorld)
{
    return renderPhong(viewportNDC, uniforms, serializedWorld);
}
