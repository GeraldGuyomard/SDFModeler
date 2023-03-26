//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Uniforms.h"
#include "SDFResult.h"
#include "PrimitiveEvaluator.h"

#include "Composition.h"


using Composition = SDFComposition<RSTTransformer, ConstMaterial>;



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
        
        CONSTANT uint8_t* buffer = &_serializedScene.buffer[0];
        CONSTANT ObjectHeader* header = reinterpret_cast<CONSTANT ObjectHeader*>(buffer);
        
        for (size_t i=0; i < _serializedScene.objectCount; ++i)
        {
            const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, header);
            if (!culled)
            {
                objectsList.headers[objectsList.nbObjects++] = header;
            }
            
            header = ObjectHeader::next(header);
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
    
    CONSTANT SerializedScene& _serializedScene;
    TShader _shader;
};
