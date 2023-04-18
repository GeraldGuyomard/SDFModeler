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
#include "ComputeShade.h"

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
        ObjectHeadersArray headersArray;
        
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
                const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, headerToCull);
                if (!culled)
                {
                    ++nbPositiveObjects;
                    headersArray.headers[headersArray.nbObjects++] = headerToCull;
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
                        headersArray.headers[headersArray.nbObjects++] = headerToCull;
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
        
        if (hasNegativeObjects)
        {
            for (size_t i=0; i < kNbSteps; ++i)
            {
                pt = ray.pt(d);
                
                minDistance = 1e5f;
                minObjectHeaderIndex = -1;
                
                size_t objectIndex = 0;
                
                while (objectIndex < headersArray.nbObjects)
                {
                    const auto startIndex = objectIndex;
                    
                    const auto res = computeDistance(pt, headersArray, objectIndex);
                    const float dist = res.distance;
                    objectIndex = res.objectIndex;
                    
                    CONSTANT ObjectHeader* startHeader = headersArray.headers[startIndex];
                    
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
                
                for (size_t objectIndex = 0; objectIndex < headersArray.nbObjects; ++objectIndex)
                {
                    CONSTANT ObjectHeader* header = headersArray.headers[objectIndex];
                    
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
            if (!hit || (outlineHeaderIndex != minObjectHeaderIndex))
            {
                CONSTANT ObjectHeader* minHeader = headersArray.headers[minObjectHeaderIndex];
                return RayMarchResult { ray, minHeader->objectId, float4{ 1, 1, 1, 1 }, 0.f };
            }
        }
        
        if (hit)
        {
            const float4 color = computeShade(_shader, ray, minDistance, pt, headersArray, minObjectHeaderIndex);
            
            CONSTANT ObjectHeader* minHeader = headersArray.headers[minObjectHeaderIndex];
            
            return RayMarchResult { ray, minHeader->objectId, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
