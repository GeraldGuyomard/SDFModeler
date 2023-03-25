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

template <typename TEvaluator, typename TPrimitive, typename TReturnValue>
INLINE TReturnValue evaluatePrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    CONSTANT uint8_t* firstBytePtr = &(header->firstByte);
    CONSTANT TPrimitive* prim = (CONSTANT TPrimitive*) firstBytePtr;
    const TPrimitive p = *prim;
    return evaluator.evaluate(header, p);
}

template <typename TEvaluator, typename TReturnValue>
INLINE TReturnValue evaluateAnonymousPrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    const ObjectType type = header->objectType;
    switch(type)
    {
        case ObjectType::sphere: return evaluatePrimitive<TEvaluator, Sphere, TReturnValue>(evaluator, header);
        case ObjectType::box: return evaluatePrimitive<TEvaluator, Box, TReturnValue>(evaluator, header);;
        case ObjectType::roundedBox: return evaluatePrimitive<TEvaluator, RoundedBox, TReturnValue>(evaluator, header);
        case ObjectType::plane: return evaluatePrimitive<TEvaluator, Plane, TReturnValue>(evaluator, header);

        //CASE_EVALUATE(evaluator, header, ObjectType::compositeUnion, CompositeUnion)
        default: break;
    }
    
    return {};
}

struct SerializedScene final
{
    uint64_t objectCount = 0;
    
    // buffer is an array of SerializedObject
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
        _distance = primitive.computeDistance(_pt);
        return _distance;
    }
    
    float returnValue() const
    {
        return _distance;
    }
    
private:
    const float3 _pt;
    float _distance;
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
        _color = _shader.computeShade(primitive, _ray, _distance, _pt);
        return _color;
    }
    
    float4 returnValue() const
    {
        return _color;
    }
    
private:
    const Ray _ray;
    const float _distance;
    const float3 _pt;
    const TShader _shader;
    float4 _color;
};


template <typename TShader>
class Scene final
{
public:
    
    constexpr static CONSTANT size_t kNbObjectsMax = 16;
    
    Scene(TShader shader, CONSTANT SerializedScene& serializedScene)
    : _serializedScene(serializedScene), _shader(shader)
    {}
    
    template <typename TEvaluator, typename TReturn>
    TReturn evaluate(TEvaluator evaluator) const
    {
        CONSTANT uint8_t* ptr = &_serializedScene.buffer[0];
        
        for (size_t i=0; i < _serializedScene.objectCount; ++i)
        {
            CONSTANT ObjectHeader* header = (CONSTANT ObjectHeader*)ptr;
            
            evaluateAnonymousPrimitive<TEvaluator, TReturn>(evaluator, header);
            
            ptr += header->byteSize;
        }
        
        return evaluator.returnValue();
    }
    
    SDFResult rayMarch(Ray ray) const
    {
        ObjectsList objectsList;
        
        CONSTANT uint8_t* ptr = &_serializedScene.buffer[0];
        for (size_t i=0; i < _serializedScene.objectCount; ++i)
        {
            CONSTANT ObjectHeader* header = (CONSTANT ObjectHeader*)ptr;
            
            CullEvaluator cullEvaluator { ray };
            const bool culled = evaluateAnonymousPrimitive<CullEvaluator, bool>(cullEvaluator, header);
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
            
            for (size_t objectIndex = 0; objectIndex < objectsList.nbObjects; ++objectIndex)
            {
                CONSTANT ObjectHeader* header = objectsList.headers[objectIndex];
                
                DistanceEvaluator distanceEvaluator { pt };
                const float dist = evaluateAnonymousPrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
                
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
                const float4 color = evaluateAnonymousPrimitive<MyShaderEvaluator, float4>(shadeEvaluator, minHeader);
                
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
    
    struct ObjectsList
    {
        size_t nbObjects = 0;
        CONSTANT ObjectHeader* headers[kNbObjectsMax];
    };
    
    class CullEvaluator
    {
    public:
        CullEvaluator(Ray ray)
        : _ray(ray)
        {}
        
        template <typename TPrimitive>
        bool evaluate(CONSTANT ObjectHeader* header, TPrimitive prim)
        {
            _culling = prim.evaluateCulling(_ray);
            return _culling;
        }
        
        bool returnValue() const
        {
            return _culling;
        }
        
    private:
        Ray _ray;
        bool _culling;
    };

    CONSTANT SerializedScene& _serializedScene;
    TShader _shader;
};
