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

using Composition = SDFComposition<RSTTransformer>;

template <typename TEvaluator, typename TReturnValue>
INLINE TReturnValue evaluatePrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    if (header->objectCode == computeObjectCode<Composition, RSTTransformer>())
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

struct SerializedObjects final
{
    uint64_t objectCount = 0;
    
    uint64_t padding;
    
    // should be aligned on 16 bytes
    // for SSE float moves
    
    // buffer is an array of serialized objects
    // that starts with ObjectHeaders
    uint8_t buffer[16536];
};

struct SerializedWorld final
{
    SerializedObjects content;
};

#define SURF_DIST 0.000001

template <typename TShader>
class Objects final
{
public:
    
    constexpr static CONSTANT size_t kNbObjectsMax = 128;
    
    Objects(TShader shader, CONSTANT SerializedObjects& serializedObjects)
    : _serializedObjects(serializedObjects), _shader(shader)
    {}
    
    RayMarchResult rayMarch(Ray ray) const
    {
        size_t nbObjects = 0;
        CONSTANT ObjectHeader* headers[kNbObjectsMax];
        
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT uint8_t* buffer = &_serializedObjects.buffer[0];
        CONSTANT ObjectHeader* header = reinterpret_cast<CONSTANT ObjectHeader*>(buffer);
        
        for (size_t i=0; i < _serializedObjects.objectCount; ++i)
        {
            const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, header);
            if (!culled)
            {
                headers[nbObjects++] = header;
            }
            
            header = ObjectHeader::next(header);
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        CONSTANT ObjectHeader* outlineHeader = nullptr;
        bool hit = false;
        
        float minDistance = 1e5f;
        float prevMinDistance = minDistance;
        
        float3 pt = ray.origin;
        CONSTANT ObjectHeader* minHeader = nullptr;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            minDistance = 1e5f;
            minHeader = nullptr;
            DistanceEvaluator distanceEvaluator { pt };
            
            for (size_t objectIndex = 0; objectIndex < nbObjects; ++objectIndex)
            {
                CONSTANT ObjectHeader* header = headers[objectIndex];
                
                const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
                
                if ((outlineHeader == nullptr) && header->selected)
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineHeader = header;
                    }
                }
                
                if (dist <= minDistance)
                {
                    minDistance = dist;
                    minHeader = header;
                }
            }
            
            if (minDistance <= kDistanceEpsilon)
            {
                hit = true;
                break;
            }
            
            d += minDistance;
            
            if (d > ray.maxLength)
            {
                break;
            }
            
            prevMinDistance = minDistance;
        }
        
        if (outlineHeader != nullptr)
        {
            if (!hit || (outlineHeader != minHeader))
            {
                return RayMarchResult { ray, minHeader->objectId, float4{ 1, 1, 1, 1 }, 0.f };
            }
        }
        
        if (hit)
        {
            using MyShaderEvaluator = ShadeEvaluator<TShader>;
            MyShaderEvaluator shadeEvaluator { ray, minDistance, pt, _shader };
            const float4 color = evaluatePrimitive<MyShaderEvaluator, float4>(shadeEvaluator, minHeader);
            
            return RayMarchResult { ray, minHeader->objectId, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedObjects& _serializedObjects;
    TShader _shader;
};
