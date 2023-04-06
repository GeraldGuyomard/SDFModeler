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
    Plane grid({}, { float3(-0.5f) });
    
    SDFResult res;
    res.distance = grid.raycast(ray);
    if ((res.distance >= 0) && (res.distance <= ray.maxLength))
    {
        const float3 p = ray.pt(res.distance);
        
        constexpr float kGridGreyLevel = 0.5f;
        const float4 color{ kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, kGridGreyLevel };
        GridMaterial mat(0.5f, color);
        
        res.color = mat.computeAlbedo(ray, res.distance, p);
        return res;
    }
    else
    {
        return {};
    }
}

template <typename TShader>
INLINE SDFResult computeSDF(float2 viewportNDC,
                            CONSTANT Uniforms& uniforms,
                            CONSTANT SerializedWorld& serializedWorld,
                            CONSTANT Materials& materials)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    const TShader shader { uniforms, materials };
    
    Objects<TShader> content { shader, serializedWorld.content };
    const auto contentRes = content.rayMarch(ray);
    const auto envRes = rayMarchEnvironment(shader, ray, serializedWorld);
    
    if (contentRes.isValid())
    {
        if (envRes.isColorValid())
        {
            return (contentRes.distance <= envRes.distance) ? contentRes : envRes;
        }
        else
        {
            return contentRes;
        }
    }
    else if (envRes.isColorValid())
    {
        return envRes;
    }
    
    return {};
}

template <typename TShader>
INLINE float4 render(float2 viewportNDC,
                     CONSTANT Uniforms& uniforms,
                     CONSTANT SerializedWorld& serializedWorld,
                     CONSTANT Materials& materials)
{
    const auto res = computeSDF<TShader>(viewportNDC, uniforms, serializedWorld, materials);
    if (res.isColorValid())
    {
        return res.color;
    }
    
    // Background
    //const float grey = max(ray.direction.y, 0.f);
    const float grey = 0.f;
    const float4 c = { grey, grey, grey, 1.f };
    return c;
}

INLINE float4 renderPhong(float2 viewportNDC,
                          CONSTANT Uniforms& uniforms,
                          CONSTANT SerializedWorld& serializedWorld,
                          CONSTANT Materials& materials)
{
    return render<PhongShader>(viewportNDC, uniforms, serializedWorld, materials);
}

INLINE float4 renderCellShaded(float2 viewportNDC,
                               CONSTANT Uniforms& uniforms,
                               CONSTANT SerializedWorld& serializedWorld,
                               CONSTANT Materials& materials)
{
    return render<CellShader>(viewportNDC, uniforms, serializedWorld, materials);
}

INLINE float4 renderDefault(float2 viewportNDC,
                            CONSTANT Uniforms& uniforms,
                            CONSTANT SerializedWorld& serializedWorld,
                            CONSTANT Materials& materials)
{
    return renderPhong(viewportNDC, uniforms, serializedWorld, materials);
}

INLINE PickResult pickObject(float2 viewportNDC,
                           CONSTANT Uniforms& uniforms,
                           CONSTANT SerializedWorld& serializedWorld,
                           CONSTANT Materials& materials)
{
    const auto res = computeSDF<NoShader>(viewportNDC, uniforms, serializedWorld, materials);
    if (res.objectID != 0)
    {
        Ray ray = Ray::make(viewportNDC, uniforms);
        
        PickResult result;
        result.objectID = res.objectID;
        result.position = ray.pt(res.distance);
        
        return result;
    }
    else
    {
        return {};
    }
}
