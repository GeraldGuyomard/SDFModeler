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
#include "FragmentShader/MattingShader.h"

#include <TargetConditionals.h>

template <typename TShader>
class NullEnvironment final
{
public:
    RayMarchResult rayMarch(TShader shader, Ray ray, CONSTANT SerializedWorldObject& serializedWorld) const
    {
        return { ray };
    }
};

template <typename TShader>
class GridEnvironment final
{
public:
    RayMarchResult rayMarch(TShader shader, Ray ray, CONSTANT SerializedWorldObject& serializedWorld) const
    {
       const auto grid = serializedWorld.grid;
        
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
};

template <typename TShader, typename TEnvironment = GridEnvironment<TShader>>
INLINE RayMarchResult rayMarch(float2 ndcPosition,
                            CONSTANT Uniforms& uniforms,
                            CONSTANT SerializedWorldObject& serializedWorld,
                            CONSTANT Materials& materials)
{
    const auto ray = Ray::make(ndcPosition, uniforms);
    
    const TShader shader { uniforms, materials };
    
    WorldObject<TShader> worldObject { shader, serializedWorld };
    const auto worldRes = worldObject.rayMarch(ndcPosition, uniforms.viewportSize, ray);
    
    TEnvironment environment;
    const auto envRes = environment.rayMarch(shader, ray, serializedWorld);
    //const RayMarchResult envRes { ray };
    
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

struct RenderResult final
{
    const float4 color;
    const float depth;
    
    float adjustedDepth(bool inverseZ) const
    {
        return (inverseZ) ? (1.f - depth) : depth;
    }
    
    RenderResult(float4 color, float depth)
    : color(color), depth(depth)
    {}
};

template <typename TShader, typename TEnvironment, bool writeToDepth = true>
INLINE RenderResult render(float2 viewportNDC,
                     CONSTANT Uniforms& uniforms,
                     CONSTANT SerializedWorldObject& serializedWorld,
                     CONSTANT Materials& materials)
{
    const auto res = rayMarch<TShader, TEnvironment>(viewportNDC, uniforms, serializedWorld, materials);
    if (res.isValid())
    {
        if constexpr (writeToDepth)
        {
            const float3 pt = res.ray.pt(res.distance);
            
            const float4 proj = uniforms.worldTransformToNdc() * float4 { pt.x, pt.y, pt.z, 1 };
            const float z = 1.f - (proj.z / proj.w);
            
            return { res.color, z };
        }
        else
        {
            return { res.color, 0 };
        }
    }
    
    // Background
    //const float grey = max(ray.direction.y, 0.f);
    const float grey = 0.f;
    const float4 c = { grey, grey, grey, 1.f };
    return RenderResult { c, 1.f };
}

INLINE RenderResult renderDefault(float2 viewportNDC,
                            CONSTANT Uniforms& uniforms,
                            CONSTANT SerializedWorldObject& serializedWorld,
                            CONSTANT Materials& materials)
{
    return render<PhongShader, GridEnvironment<PhongShader>>(viewportNDC, uniforms, serializedWorld, materials);
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
