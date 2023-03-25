//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Uniforms.h"
#include "SDFResult.h"

#include "Transformer/StandardTransformers.h"
#include "Material/ConstMaterial.h"
#include "Material/GridMaterial.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"
#include "SDFGeometry/SDFSubstraction.h"

#include "SDFObject.h"

using Sphere = SDFObject<SDFSphere, RSTTransformer, ConstMaterial>;
using Plane = SDFObject<SDFPlane, RSTTransformer, ConstMaterial>;
using Grid = SDFObject<SDFPlane, RSTTransformer, GridMaterial>;
using Box = SDFObject<SDFBox, RSTTransformer, ConstMaterial>;
using RoundedBox = SDFObject<SDFRoundedBox, RSTTransformer, ConstMaterial>;

#define EVALUATE(evaluator, header, TPrimitive) { \
    CONSTANT uint8_t* firstBytePtr = &(header->firstByte); \
    CONSTANT TPrimitive* prim = (CONSTANT TPrimitive*) firstBytePtr; \
    const TPrimitive p = *prim; \
    evaluator.evaluate(header, p); }\

#define CASE_EVALUATE(evaluator, header, objectType, TPrimitive) \
case objectType: EVALUATE(evaluator, header, TPrimitive); break; \

#define SWITCH_EVALUATOR(evaluator, header) { \
    const ObjectType type = header->objectType; \
    switch(type) { \
        CASE_EVALUATE(evaluator, header, ObjectType::sphere, Sphere) \
        CASE_EVALUATE(evaluator, header, ObjectType::box, Box) \
        CASE_EVALUATE(evaluator, header, ObjectType::roundedBox, RoundedBox) \
        CASE_EVALUATE(evaluator, header, ObjectType::plane, Plane) \
        default: break; \
    } \
} \

struct ObjectHeader final
{
    size_t    byteSize;
    ObjectType  objectType;
    
    uint8_t     firstByte;
    
    ObjectHeader(uint32_t byteSize, ObjectType objectType)
    : byteSize(byteSize), objectType(objectType)
    {}
};

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
    void evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive)
    {
        _distance = primitive.computeDistance(_pt);
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
    void evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive)
    {
        _color = _shader.computeShade(primitive, _ray, _distance, _pt);
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
            
            SWITCH_EVALUATOR(evaluator, header);
            
            ptr += header->byteSize;
        }
        
        return evaluator.returnValue();
    }
    
    SDFResult rayMarch(Ray ray) const
    {
        BuildObjectsListEvaluator buildEvaluator { ray };
        const auto objectsList = evaluate<BuildObjectsListEvaluator, ObjectsList>(buildEvaluator);
        
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
                SWITCH_EVALUATOR(distanceEvaluator, header);
                
                const float dist = distanceEvaluator.returnValue();
                
                if (dist <= minDistance)
                {
                    minDistance = dist;
                    minHeader = header;
                }
            }
            
            if ((minDistance >= 0.f) && (minDistance <= kDistanceEpsilon))
            {
                ShadeEvaluator<TShader> shadeEvaluator { ray, minDistance, pt, _shader };
                SWITCH_EVALUATOR(shadeEvaluator, minHeader);
                const float4 color = shadeEvaluator.returnValue();
                
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
    
    class BuildObjectsListEvaluator
    {
    public:
        BuildObjectsListEvaluator(Ray ray)
        : _ray(ray)
        {}
        
        template <typename TPrimitive>
        void evaluate(CONSTANT ObjectHeader* header, TPrimitive prim)
        {
            if (!prim.evaluateCulling(_ray))
            {
                // possibly intersecting ray
                _objectsList.headers[_objectsList.nbObjects++] = header;
            }
        }
        
        ObjectsList returnValue() const
        {
            return _objectsList;
        }
        
    private:
        Ray _ray;
        ObjectsList _objectsList;
    };

    CONSTANT SerializedScene& _serializedScene;
    TShader _shader;
};
