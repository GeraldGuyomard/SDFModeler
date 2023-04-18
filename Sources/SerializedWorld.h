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

using Composition = SDFComposition;

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

struct SerializedWorld final
{
    uint64_t objectCount = 0;
    
    uint64_t padding;
    
    // should be aligned on 16 bytes
    // for SSE float moves
    
    // buffer is an array of serialized objects
    // that starts with ObjectHeaders
    uint8_t buffer[16536];
};

template <typename TShader>
class Content final
{
public:
    
    constexpr static CONSTANT size_t kNbObjectsMax = 128;
    
    Content(TShader shader, CONSTANT SerializedWorld& serializedWorld)
    : _serializedWorld(serializedWorld), _shader(shader)
    {}
    
    RayMarchResult rayMarch(Ray ray) const
    {
        size_t nbObjects = 0;
        CONSTANT ObjectHeader* headers[kNbObjectsMax];
        
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT uint8_t* buffer = &_serializedWorld.buffer[0];
        CONSTANT ObjectHeader* headerToCull = reinterpret_cast<CONSTANT ObjectHeader*>(buffer);
        
        int64_t nbObjectsLeftToCull = _serializedWorld.objectCount;
        
        while (nbObjectsLeftToCull > 0)
        {
            const auto objectID = headerToCull->objectId;
            
            // cull first positive object
            size_t nbPositiveObjects = 0;
            while ((nbObjectsLeftToCull > 0) &&
                   (headerToCull->objectId == objectID) &&
                   (headerToCull->sdfOperation() == SDFOperation::addition))
            {
                const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, headerToCull);
                if (!culled)
                {
                    ++nbPositiveObjects;
                    headers[nbObjects++] = headerToCull;
                }
                
                --nbObjectsLeftToCull;
                headerToCull = ObjectHeader::next(headerToCull);
            }
            
            if (nbPositiveObjects == 0)
            {
                // remove any negative objects
                while ((nbObjectsLeftToCull > 0) &&
                       (headerToCull->objectId == objectID) &&
                       (headerToCull->sdfOperation() == SDFOperation::substraction))
                {
                    --nbObjectsLeftToCull;
                    headerToCull = ObjectHeader::next(headerToCull);
                }
            }
            else
            {
                // cull all the negative parts
                while ((nbObjectsLeftToCull > 0) &&
                       (headerToCull->objectId == objectID) &&
                       (headerToCull->sdfOperation() == SDFOperation::substraction))
                {
                    const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, headerToCull);
                    if (!culled)
                    {
                        headers[nbObjects++] = headerToCull;
                    }
                    
                    --nbObjectsLeftToCull;
                    headerToCull = ObjectHeader::next(headerToCull);
                }
            }
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
            
            size_t objectIndex = 0;
            
            while (objectIndex < nbObjects)
            {
                CONSTANT ObjectHeader* startHeader = headers[objectIndex];
                const auto objectID = startHeader->objectId;
                float2 distances { 1e7f, 1e7f };
                
                CONSTANT ObjectHeader* h = startHeader;
                
                while ((objectIndex < nbObjects) && (h->objectId == objectID))
                {
                    const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, h);
                    distances[h->operation] = min(distances[h->operation], dist);
                    
                    h = headers[++objectIndex];
                }
                
                const float dist = max(distances.x, -distances.y);
                
                if (startHeader->selected && (outlineHeader == nullptr))
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineHeader = startHeader;
                    }
                }
                
                if (dist <= minDistance)
                {
                    minDistance = dist;
                    minHeader = startHeader;
                }
                
                //++objectIndex;
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
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
