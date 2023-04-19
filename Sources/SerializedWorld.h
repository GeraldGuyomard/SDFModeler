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

#include "ComputeDistance.h"
#include "ShadedPrimitive.h"

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
class WorldObject final
{
public:
    
    WorldObject(TShader shader, CONSTANT SerializedWorld& serializedWorld)
    : _serializedWorld(serializedWorld), _shader(shader)
    {}
    
    RayMarchResult rayMarch(Ray ray) const
    {
        CONSTANT uint8_t* buffer = &_serializedWorld.buffer[0];
        
        ObjectHeadersArray headersArray { buffer };
        
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT ObjectHeader* headerToCull = reinterpret_cast<CONSTANT ObjectHeader*>(buffer);
        
        int64_t nbObjectsLeftToCull = _serializedWorld.objectCount;
        bool hasNegativeObjects = false;
        
        while (nbObjectsLeftToCull > 0)
        {
            const auto objectID = headerToCull->objectId;
            
            // cull first positive object
            bool hasPositiveObjects = false;
            while ((nbObjectsLeftToCull > 0) &&
                   (headerToCull->objectId == objectID) &&
                   (headerToCull->sdfOperation() == SDFOperation::addition))
            {
                const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, headerToCull);
                if (!culled)
                {
                    hasPositiveObjects = true;
                    headersArray.add(headerToCull);
                }
                
                --nbObjectsLeftToCull;
                headerToCull = ObjectHeader::next(headerToCull);
            }
            
            if (!hasPositiveObjects)
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
                        headersArray.add(headerToCull);
                        hasNegativeObjects = true;
                    }
                    
                    --nbObjectsLeftToCull;
                    headerToCull = ObjectHeader::next(headerToCull);
                }
            }
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        int64_t outlineHeaderIndex = -1;
        bool hit = false;
        
        float minDistance = 1e5f;
        float prevMinDistance = minDistance;
        
        float3 pt = ray.origin;
        int64_t minObjectHeaderIndex = -1;
        
        const size_t nbObjects = headersArray.nbObjects();
        
        if (hasNegativeObjects)
        {
            for (size_t i=0; i < kNbSteps; ++i)
            {
                pt = ray.pt(d);
                
                minDistance = 1e5f;
                minObjectHeaderIndex = -1;
                
                size_t objectIndex = 0;
                
                while (objectIndex < nbObjects)
                {
                    const auto startIndex = objectIndex;
                    
                    const float dist = computeDistance(pt, headersArray, objectIndex);
                    
                    CONSTANT ObjectHeader* startHeader = headersArray.header(startIndex);
                    
                    if (startHeader->selected && (outlineHeaderIndex < 0))
                    {
                        if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                        {
                            outlineHeaderIndex = startIndex;
                        }
                    }
                    
                    if (dist <= minDistance)
                    {
                        minDistance = dist;
                        minObjectHeaderIndex = startIndex;
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
                minObjectHeaderIndex = -1;
                DistanceEvaluator distanceEvaluator { pt };
                
                for (size_t objectIndex = 0; objectIndex < nbObjects; ++objectIndex)
                {
                    CONSTANT ObjectHeader* header = headersArray.header(objectIndex);
                    
                    const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, header);
                    
                    if (header->selected && (outlineHeaderIndex < 0))
                    {
                        if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                        {
                            outlineHeaderIndex = objectIndex;
                        }
                    }
                    
                    if (dist <= minDistance)
                    {
                        minDistance = dist;
                        minObjectHeaderIndex = objectIndex;
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
        
        if (outlineHeaderIndex >= 0)
        {
            if (!hit)
            {
                CONSTANT ObjectHeader* minHeader = headersArray.header(minObjectHeaderIndex);
                return RayMarchResult { ray, minHeader->objectId, float4{ 1, 1, 1, 1 }, 0.f };
            }
            else if (outlineHeaderIndex != minObjectHeaderIndex)
            {
                CONSTANT ObjectHeader* outlineHeader = headersArray.header(outlineHeaderIndex);
                CONSTANT ObjectHeader* minHeader = headersArray.header(minObjectHeaderIndex);
                
                if (outlineHeader->objectId != minHeader->objectId)
                {
                    return RayMarchResult { ray, minHeader->objectId, float4{ 1, 1, 1, 1 }, 0.f };
                }
            }
        }
        
        if (hit)
        {
            ShadedPrimitive primitive { headersArray, size_t(minObjectHeaderIndex) };
            const float4 color = _shader.computeShade(primitive, ray, minDistance, pt);
            
            CONSTANT ObjectHeader* minHeader = headersArray.header(minObjectHeaderIndex);
            
            return RayMarchResult { ray, minHeader->objectId, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
