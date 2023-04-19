//
//  RenderFunctions.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/28/23.
//

#pragma once

#include "ShaderTypes.h"
#include "Ray.h"
#include "Results.h"
#include "SDFObject.h"
#include "SerializedWorldObject.h"

#include "FragmentShader/PhongShader.h"
#include "FragmentShader/CellShader.h"

template <typename TShader>
INLINE RayMarchResult rayMarchEnvironment(TShader shader, Ray ray, CONSTANT SerializedWorldObject& serializedWorld)
{
    SDFObject<SDFPlane> grid({}, { float3(-0.5f) });
    
    RayMarchResult res { ray };
    res.distance = grid.raycast(ray);
    
    if ((res.distance >= 0) && (res.distance <= ray.maxLength))
    {
        constexpr float kGridGreyLevel = 0.5f;
        const float4 color{ kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, kGridGreyLevel };
        GridMaterial mat(0.5f, color);
        
        const auto pt = ray.pt(res.distance);
        res.color = mat.computeAlbedo(ray, res.distance, pt);
        return res;
    }
    else
    {
        return { ray };
    }
}

template <typename TShader>
INLINE RayMarchResult rayMarch(float2 viewportNDC,
                            CONSTANT Uniforms& uniforms,
                            CONSTANT SerializedWorldObject& serializedWorld,
                            CONSTANT Materials& materials)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    const TShader shader { uniforms, materials };
    
    WorldObject<TShader> worldObject { shader, serializedWorld };
    const auto worldRes = worldObject.rayMarch(ray);
    const auto envRes = rayMarchEnvironment(shader, ray, serializedWorld);
    
    if (worldRes.isValid())
    {
        if (envRes.isValid())
        {
            return (worldRes.distance <= envRes.distance) ? worldRes : envRes;
        }
        else
        {
            return worldRes;
        }
    }
    else if (envRes.isValid())
    {
        return envRes;
    }
    
    return { ray };
}

template <typename TShader>
INLINE float4 render(float2 viewportNDC,
                     CONSTANT Uniforms& uniforms,
                     CONSTANT SerializedWorldObject& serializedWorld,
                     CONSTANT Materials& materials)
{
    const auto res = rayMarch<TShader>(viewportNDC, uniforms, serializedWorld, materials);
    if (res.isValid())
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
                          CONSTANT SerializedWorldObject& serializedWorld,
                          CONSTANT Materials& materials)
{
    return render<PhongShader>(viewportNDC, uniforms, serializedWorld, materials);
}

INLINE float4 renderCellShaded(float2 viewportNDC,
                               CONSTANT Uniforms& uniforms,
                               CONSTANT SerializedWorldObject& serializedWorld,
                               CONSTANT Materials& materials)
{
    return render<CellShader>(viewportNDC, uniforms, serializedWorld, materials);
}

INLINE float4 renderDefault(float2 viewportNDC,
                            CONSTANT Uniforms& uniforms,
                            CONSTANT SerializedWorldObject& serializedWorld,
                            CONSTANT Materials& materials)
{
    return renderPhong(viewportNDC, uniforms, serializedWorld, materials);
}

INLINE PickResult pickObject(float2 viewportNDC,
                           CONSTANT Uniforms& uniforms,
                           CONSTANT SerializedWorldObject& serializedWorld,
                           CONSTANT Materials& materials)
{
    const auto res = rayMarch<NoShader>(viewportNDC, uniforms, serializedWorld, materials);
    if (res.objectID != 0)
    {
        Ray ray = Ray::make(viewportNDC, uniforms);
        
        return { res.objectID, ray.pt(res.distance) };
    }
    else
    {
        return {};
    }
}
