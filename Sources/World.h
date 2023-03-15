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

#include "FragmentShader/PhongShader.h"
#include "FragmentShader/CellShader.h"

#include "Transformer/StandardTransformers.h"
#include "Material/ConstMaterial.h"
#include "Material/GridMaterial.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"
#include "SDFGeometry/SDFSubstraction.h"

using Sphere = SDFObject<SDFSphere, RSTTransformer, ConstMaterial>;
using Plane = SDFObject<SDFPlane, RSTTransformer, ConstMaterial>;
using Grid = SDFObject<SDFPlane, RSTTransformer, GridMaterial>;
using Box = SDFObject<SDFBox, RSTTransformer, ConstMaterial>;
using RoundedBox = SDFObject<SDFRoundedBox, RSTTransformer, ConstMaterial>;

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
class DynamicObject final
{
public:
    DynamicObject(CONSTANT DynamicScene& mutableState)
    : _dynamicScene(mutableState)
    {}
    
    bool culled() const
    {
        return false;
    }
    
    float computeDistance(float3 p) const
    {
        CONSTANT uint8_t* ptr = &_dynamicScene.buffer[0];
        CONSTANT uint8_t* end = ptr + _dynamicScene.size;
        
        while (ptr < end)
        {
            CONSTANT ObjectType* pType = (CONSTANT ObjectType*)ptr;
            
            const ObjectType type = *pType;
            ptr += sizeof(type);
            
            switch(type)
            {
                case ObjectType::sphere:
                {
                    CONSTANT Sphere* sphere = (CONSTANT Sphere*) ptr;
                    CONSTANT uint8_t* dataEnd = ptr + sizeof(Sphere);
                    
                    //return sphere->computeDistance(p);
                    
                    ptr = dataEnd;
                    break;
                }
                    
                case ObjectType::box:
                {
                    break;
                }
            }
        }
        
        return {};
    }
    
    float4 computeAlbedo(float3 p) const
    {
        return { 1, 0, 0, 1};
    }
    
    SDFResult rayMarch(Ray ray, TShader shader) const
    {
        constexpr int kNbSteps = 100;
        
        float d = 0.f;
        
        for (int i=0; i < kNbSteps; ++i)
        {
            float3 p = ray.pt(d);
            auto result = computeSDF(p, ray, shader, *this);
            
            if (result.hit())
            {
                return result;
            }
            
            d += result.distance;
            
            if (d > ray.maxLength)
            {
                break;
            }
        }
        
        return {};
    }
    
private:
    CONSTANT DynamicScene& _dynamicScene;
};

template <typename TShader>
INLINE float4 render(float2 viewportNDC, TShader shader, CONSTANT Uniforms& uniforms, CONSTANT DynamicScene& mutableState)
{
    const auto ray = Ray::make(viewportNDC, uniforms);
    
    DynamicObject<TShader> dynObject { mutableState };
    dynObject.rayMarch(ray, shader);
    
    Scene scene;
    auto res = scene.rayMarch(ray, shader);
    if (res.isValid())
    {
        return res.color;
    }

    // Test Env Last
    Environment env;
    res = env.rayMarch(ray, shader);

    return res.color;
}

