//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Uniforms.h"
#include "SDFResult.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"
#include "SDFGeometry/SDFSubstraction.h"
#include "Composition.h"

#include "SDFObject.h"

#include "Transformer/StandardTransformers.h"
#include "Material/ConstMaterial.h"
#include "Material/GridMaterial.h"

using Sphere = SDFObject<SDFSphere, RSTTransformer, ConstMaterial>;
using Plane = SDFObject<SDFPlane, RSTTransformer, ConstMaterial>;
using Grid = SDFObject<SDFPlane, RSTTransformer, GridMaterial>;
using Box = SDFObject<SDFBox, RSTTransformer, ConstMaterial>;
using RoundedBox = SDFObject<SDFRoundedBox, RSTTransformer, ConstMaterial>;

using Composition = SDFComposition<RSTTransformer, ConstMaterial>;

template <typename TEvaluator, typename TReturnValue>
INLINE TReturnValue evaluateAtomicPrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    const ObjectType type = header->objectType;
    switch(type)
    {
        case ObjectType::sphere: return evaluateTypedPrimitive<TEvaluator, Sphere, TReturnValue>(evaluator, header);
        case ObjectType::box: return evaluateTypedPrimitive<TEvaluator, Box, TReturnValue>(evaluator, header);;
        case ObjectType::roundedBox: return evaluateTypedPrimitive<TEvaluator, RoundedBox, TReturnValue>(evaluator, header);
        case ObjectType::plane: return evaluateTypedPrimitive<TEvaluator, Plane, TReturnValue>(evaluator, header);

        default: break;
    }
    
    return {};
}

template <typename TEvaluator, typename TReturnValue>
INLINE TReturnValue evaluatePrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    if (header->objectType == ObjectType::composition)
    {
        auto serializedComposition = typedPrimitive<Composition::Serialized>(header);
        Composition composition(serializedComposition);
        
        return evaluator.evaluate(header, composition);
    }
    else
    {
        return evaluateAtomicPrimitive<TEvaluator, TReturnValue>(evaluator, header);
    }
}

struct SerializedScene final
{
    uint64_t objectCount = 0;
    
    uint64_t padding;
    
    // should be aligned on 16 bytes
    // for SSE float moves
    
    // buffer is an array of serialized objects
    // that starts with ObjectHeaders
    uint8_t buffer[2048];
};

class DistanceEvaluator
{
public:
    
    DistanceEvaluator(float3 pt)
    : _pt(pt)
    {}
    
    template <typename TPrimitive>
    float evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive)
    {
        return primitive.computeDistance(_pt);
    }
    
private:
    const float3 _pt;
};

template <typename TShader>
class ShadeEvaluator
{
public:
    
    ShadeEvaluator(Ray ray, float distance, float3 pt, TShader shader)
    : _ray(ray), _distance(distance), _pt(pt), _shader(shader)
    {}
    
    template <typename TPrimitive>
    float4 evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive)
    {
        return _shader.computeShade(primitive, _ray, _distance, _pt);
    }
    
private:
    const Ray _ray;
    const float _distance;
    const float3 _pt;
    const TShader _shader;
};


template <typename TShader>
class Scene final
{
public:
    
    constexpr static CONSTANT size_t kNbObjectsMax = 16;
    
    Scene(TShader shader, CONSTANT SerializedScene& serializedScene)
    : _serializedScene(serializedScene), _shader(shader)
    {}
    
    SDFResult rayMarch(Ray ray) const
    {
        struct ObjectsList
        {
            size_t nbObjects = 0;
            CONSTANT ObjectHeader* headers[kNbObjectsMax];
        };
        
        ObjectsList objectsList;
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT uint8_t* ptr = &_serializedScene.buffer[0];
        for (size_t i=0; i < _serializedScene.objectCount; ++i)
        {
            CONSTANT ObjectHeader* header = (CONSTANT ObjectHeader*)ptr;
            
            const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, header);
            if (!culled)
            {
                objectsList.headers[objectsList.nbObjects++] = header;
            }
            
            ptr += header->byteSize;
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            float3 pt = ray.pt(d);
            
            float minDistance = 10000.f;
            CONSTANT ObjectHeader* minHeader = nullptr;
            DistanceEvaluator distanceEvaluator { pt };
            
            for (size_t objectIndex = 0; objectIndex < objectsList.nbObjects; ++objectIndex)
            {
                CONSTANT ObjectHeader* header = objectsList.headers[objectIndex];
                
                const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
                
                if (dist <= minDistance)
                {
                    minDistance = dist;
                    minHeader = header;
                }
            }
            
            if ((minDistance >= 0.f) && (minDistance <= kDistanceEpsilon))
            {
                using MyShaderEvaluator = ShadeEvaluator<TShader>;
                MyShaderEvaluator shadeEvaluator { ray, minDistance, pt, _shader };
                const float4 color = evaluatePrimitive<MyShaderEvaluator, float4>(shadeEvaluator, minHeader);
                
                return { minDistance, color };
            }
            
            d += minDistance;
            
            if (d > ray.maxLength)
            {
                break;
            }
        }
        
        return {};
    }
    
private:
    
    class CullEvaluator
    {
    public:
        CullEvaluator(Ray ray)
        : _ray(ray)
        {}
        
        template <typename TPrimitive>
        bool evaluate(CONSTANT ObjectHeader* header, TPrimitive prim)
        {
            return prim.evaluateCulling(_ray);
        }
        
    private:
        Ray _ray;
    };

    CONSTANT SerializedScene& _serializedScene;
    TShader _shader;
};
