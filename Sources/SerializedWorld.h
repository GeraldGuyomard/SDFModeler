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
    
    struct ComputeDistanceResult
    {
        const float distance;
        const size_t objectIndex;
        
        ComputeDistanceResult(float distance, size_t objectIndex)
        : distance(distance), objectIndex(objectIndex)
        {}
    };
    
    static ComputeDistanceResult computeDistance(float3 pt, CONSTANT ObjectHeader* headers[], size_t objectIndex, size_t nbObjects)
    {
        DistanceEvaluator distanceEvaluator { pt };
        
        CONSTANT ObjectHeader* startHeader = headers[objectIndex];
        const auto objectID = startHeader->objectId;
        float2 distances { 1e7f, 1e7f };
        
        CONSTANT ObjectHeader* h = startHeader;
        
        while ((objectIndex < nbObjects) && (h->objectId == objectID))
        {
            const float dist = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, h);
            distances[h->operation] = min(distances[h->operation], dist);
            
            h = headers[++objectIndex];
        }
        
        const float dist = max(distances.x, -distances.y);
        return { dist, objectIndex };
    }
    
    RayMarchResult rayMarch(Ray ray) const
    {
        size_t nbObjects = 0;
        CONSTANT ObjectHeader* headers[kNbObjectsMax];
        
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT uint8_t* buffer = &_serializedWorld.buffer[0];
        CONSTANT ObjectHeader* headerToCull = reinterpret_cast<CONSTANT ObjectHeader*>(buffer);
        
        int64_t nbObjectsLeftToCull = _serializedWorld.objectCount;
        bool hasNegativeObjects = false;
        
        while (nbObjectsLeftToCull > 0)
        {
            const auto objectID = headerToCull->objectId;
            
            // cull first positive object
            size_t nbPositiveObjects = 0;
            while ((nbObjectsLeftToCull > 0) &&
                   (headerToCull->objectId == objectID) &&
                   (headerToCull->sdfOperation() == SDFOperation::addition))
            {
                const bool culled = evaluateAtomicPrimitive<CullEvaluator, bool>(cullEvaluator, headerToCull);
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
                    const bool culled = evaluateAtomicPrimitive<CullEvaluator, bool>(cullEvaluator, headerToCull);
                    if (!culled)
                    {
                        headers[nbObjects++] = headerToCull;
                        hasNegativeObjects = true;
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
        
        if (hasNegativeObjects)
        {
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
                    
                    const auto res = computeDistance(pt, headers, objectIndex, nbObjects);
                    const float dist = res.distance;
                    objectIndex = res.objectIndex;
                    
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
        }
        else
        {
            // only positive parts
            for (size_t i=0; i < kNbSteps; ++i)
            {
                pt = ray.pt(d);
                
                minDistance = 1e5f;
                minHeader = nullptr;
                DistanceEvaluator distanceEvaluator { pt };
                
                for (size_t objectIndex = 0; objectIndex < nbObjects; ++objectIndex)
                {
                    CONSTANT ObjectHeader* header = headers[objectIndex];
                    
                    const float dist = evaluateAtomicPrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
                    
                    if (header->selected && (outlineHeader == nullptr))
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
            const float4 color = evaluateAtomicPrimitive<MyShaderEvaluator, float4>(shadeEvaluator, minHeader);
            
            return RayMarchResult { ray, minHeader->objectId, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
