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

template <typename TShader>
class DynamicObject final
{
public:
    
    constexpr static CONSTANT size_t kNbObjectsMax = 16;
    
    DynamicObject(TShader shader, CONSTANT DynamicScene& mutableState)
    : _dynamicScene(mutableState), _shader(shader)
    {}
    
    template <typename TEvaluator, typename TReturn>
    TReturn evaluate(TEvaluator evaluator) const
    {
        CONSTANT uint8_t* ptr = &_dynamicScene.buffer[0];
        
        for (size_t i=0; i < _dynamicScene.objectCount; ++i)
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
            
            SDFResult sdfResult = { 10000, 0.f };
            
            for (size_t objectIndex = 0; objectIndex < objectsList.nbObjects; ++objectIndex)
            {
                CONSTANT ObjectHeader* header = objectsList.headers[objectIndex];
                
                ComputeSDFEvaluator sdfEvaluator { ray, _shader, pt };
                SWITCH_EVALUATOR(sdfEvaluator, header);
                
                const auto res = sdfEvaluator.returnValue();
                
                if (res.distance <= sdfResult.distance)
                {
                    sdfResult = res;
                }
            }
            
            if (sdfResult.hit())
            {
                return sdfResult;
            }
            
            d += sdfResult.distance;
            
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

    class ComputeSDFEvaluator
    {
    public:
        
        ComputeSDFEvaluator(Ray ray, TShader shader, float3 pt)
        : _ray(ray), _shader(shader), _pt(pt)
        {}
        
        template <typename TPrimitive>
        void evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive)
        {
            const float d = primitive.computeDistance(_pt);
            
            if (d > kDistanceEpsilon)
            {
                _result = { d, 0.f };
            }
            else
            {
                const float4 c = _shader.computeShade(primitive, _ray, d, _pt);
                _result = { d, c };
            }
        }
        
        SDFResult returnValue() const
        {
            return _result;
        }
        
    private:
        const Ray _ray;
        const TShader _shader;
        const float3 _pt;
        SDFResult _result;
    };
    
    CONSTANT DynamicScene& _dynamicScene;
    TShader _shader;
};
