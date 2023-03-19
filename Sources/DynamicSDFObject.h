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

template <typename TShader>
class DynamicObject final
{
public:
    DynamicObject(Ray ray, CONSTANT DynamicScene& mutableState)
    : _dynamicScene(mutableState)
    {
        
    }
    
    bool culled() const
    {
        return _culled;
    }
    
    template <typename TEvaluator, typename TReturn>
    TReturn evaluate(TEvaluator evaluator) const
    {
        CONSTANT uint8_t* ptr = &_dynamicScene.buffer[0];
        
        for (size_t i=0; i < _dynamicScene.objectCount; ++i)
        {
            CONSTANT ObjectHeader* header = (CONSTANT ObjectHeader*)ptr;
            
            const ObjectType type = header->objectType;
            CONSTANT uint8_t* firstBytePtr = &(header->firstByte);
            
            switch(type)
            {
                case ObjectType::sphere:
                {
                    CONSTANT Sphere* sphere = (CONSTANT Sphere*) firstBytePtr;
                    const Sphere s = *sphere;
                    evaluator.evaluate(s);
                    
                    break;
                }
                    
                default: break;
            }
            
            ptr += header->byteSize;
        }
        
        return evaluator.returnValue();
    }

    struct ComputeDistanceEvaluator
    {
        ComputeDistanceEvaluator(float3 p)
        : p(p)
        {}
        
        template <typename TPrimitive>
        void evaluate(TPrimitive prim)
        {
            const float d = prim.computeDistance(p);
            minDistance = min(minDistance, d);
        }
        
        float returnValue() const
        {
            return minDistance;
        }
        
        const float3 p;
        float minDistance = 1e7;
    };
    
    float computeDistance(float3 p) const
    {
        ComputeDistanceEvaluator eval(p);
        return evaluate<ComputeDistanceEvaluator, float>(eval);
    }
    
    float4 computeAlbedo(float3 p) const
    {
        return { 1, 0, 0, 1};
    }
    
private:
    CONSTANT DynamicScene& _dynamicScene;
    bool _culled = false;
};
