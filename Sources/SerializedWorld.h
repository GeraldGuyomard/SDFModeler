//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Uniforms.h"
#include "TransformedGeometryEvaluator.h"
#include "Results.h"

struct SerializedWorld final
{
    uint64_t geometriesCount = 0;
    
    uint32_t simpleObjectsCount = 0;
    uint32_t compoundObjectsCount = 0;
    
    // should be aligned on 16 bytes
    // for SSE float moves
    
    // buffer is an array of serialized objects
    // that starts with ObjectHeaders
    uint8_t geometries[16536];
    
    SimpleObjectHeader simpleObjectHeaders[128];
    uint8_t compoundObjectHeaders[16536];
};

template <typename TShader>
class Content final
{
public:
    
    constexpr static CONSTANT size_t kNbGeometriesMax = 128;
    constexpr static CONSTANT size_t kNbSimpleObjectsMax = 128;
    constexpr static CONSTANT size_t kNbCompoundObjectsMax = 128;
    
    Content(TShader shader, CONSTANT SerializedWorld& serializedWorld)
    : _serializedWorld(serializedWorld), _shader(shader)
    {}
    
    RayMarchResult rayMarch(Ray ray) const
    {
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT TransformedGeometryHeader* geometryHeaders[kNbGeometriesMax];
        
        CONSTANT uint8_t* geometriesBuffer = &_serializedWorld.geometries[0];
        CONSTANT TransformedGeometryHeader* geometryHeader = reinterpret_cast<CONSTANT TransformedGeometryHeader*>(geometriesBuffer);
        
        bool geometryCulled[kNbGeometriesMax];
        
        for (size_t i=0; i < _serializedWorld.geometriesCount; ++i)
        {
            bool culled = evaluateTransformedGeometry<CullEvaluator, bool>(cullEvaluator, geometryHeader);
            
            geometryHeaders[i] = geometryHeader;
            geometryCulled[i] = culled;
            
            geometryHeader = TransformedGeometryHeader::next(geometryHeader);
        }
        
        size_t simpleObjectsCount = 0;
        
        CONSTANT SimpleObjectHeader* simpleObjectHeaders[kNbSimpleObjectsMax];
        for (size_t i=0; i < _serializedWorld.simpleObjectsCount; ++i)
        {
            CONSTANT SimpleObjectHeader* objectHeader = &_serializedWorld.simpleObjectHeaders[i];
            if (!geometryCulled[objectHeader->geometryIndex])
            {
                simpleObjectHeaders[simpleObjectsCount++] = objectHeader;
            }
        }
        
        size_t compoundObjectsCount = 0;
        CONSTANT CompoundObjectHeader* compoundObjectHeaders[kNbCompoundObjectsMax];
        
        CONSTANT uint8_t* compoundObjectsBuffer = &_serializedWorld.compoundObjectHeaders[0];
        CONSTANT CompoundObjectHeader* compoundObjectHeader = reinterpret_cast<CONSTANT CompoundObjectHeader*>(compoundObjectsBuffer);
        
        for (size_t i=0; i < _serializedWorld.compoundObjectsCount; ++i)
        {
            CONSTANT uint32_t* indices = &compoundObjectHeader->firstPositiveIndex;
            
            for (size_t i=0; i < compoundObjectHeader->nbPositiveGeometries; ++i)
            {
                const bool culled = geometryCulled[indices[i]];
                if (!culled)
                {
                    // at least one positive geom is not culled -> the entire compound should be ray marched
                    compoundObjectHeaders[compoundObjectsCount++] = compoundObjectHeader;
                }
            }
            
            compoundObjectHeader = CompoundObjectHeader::next(compoundObjectHeader);
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        CONSTANT ObjectHeader* outlineObjectHeader = nullptr;
        bool hit = false;
        
        float minDistance = 1e5f;
        float prevMinDistance = minDistance;
        
        float3 pt = ray.origin;
        CONSTANT ObjectHeader* minObjectHeader = nullptr;
        CONSTANT TransformedGeometryHeader* minGeometryHeader = nullptr;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            minDistance = 1e5f;
            minObjectHeader = nullptr;
            DistanceEvaluator distanceEvaluator { pt };
            
            // simple objects
            for (size_t objectIndex = 0; objectIndex < simpleObjectsCount; ++objectIndex)
            {
                CONSTANT SimpleObjectHeader* simpleObjectHeader = simpleObjectHeaders[objectIndex];
                geometryHeader = geometryHeaders[simpleObjectHeader->geometryIndex];
                
                const float dist = evaluateTransformedGeometry<DistanceEvaluator, float>(distanceEvaluator, geometryHeader);
                
                if ((outlineObjectHeader == nullptr) && simpleObjectHeader->objectHeader.selected)
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineObjectHeader = &simpleObjectHeader->objectHeader;
                    }
                }
                
                if (dist < minDistance)
                {
                    minDistance = dist;
                    minObjectHeader = &simpleObjectHeader->objectHeader;
                    minGeometryHeader = geometryHeader;
                }
            }
            
            // compound objects
            for (size_t objectIndex = 0; objectIndex < compoundObjectsCount; ++objectIndex)
            {
                CONSTANT CompoundObjectHeader* compoundObjectHeader = compoundObjectHeaders[objectIndex];
                
                CONSTANT uint32_t* indices = &compoundObjectHeader->firstPositiveIndex;
                
                size_t i=0;
                
                float positiveDist = 1e7f;
                for (; i < compoundObjectHeader->nbPositiveGeometries; ++i)
                {
                    const uint32_t geometryIndex = indices[i];
                    if (geometryCulled[geometryIndex])
                    {
                        continue;
                    }
                    
                    geometryHeader = geometryHeaders[geometryIndex];
                    
                    const float geomDist = evaluateTransformedGeometry<DistanceEvaluator, float>(distanceEvaluator, geometryHeader);
                    positiveDist = min(positiveDist, geomDist);
                }
                
                float negativeDist = 1e7f;
                const uint32_t end = compoundObjectHeader->nbPositiveGeometries + compoundObjectHeader->nbNegativeGeometries;
                for (; i < end; ++i)
                {
                    const uint32_t geometryIndex = indices[i];
                    if (geometryCulled[geometryIndex])
                    {
                        continue;
                    }
                    
                    auto negativeGeometryHeader = geometryHeaders[geometryIndex];
                    
                    const float geomDist = evaluateTransformedGeometry<DistanceEvaluator, float>(distanceEvaluator, negativeGeometryHeader);
                    negativeDist = min(negativeDist, geomDist);
                }
                
                const float dist = max(positiveDist, -negativeDist);
                
                if ((outlineObjectHeader == nullptr) && compoundObjectHeader->objectHeader.selected)
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineObjectHeader = &compoundObjectHeader->objectHeader;
                    }
                }
                
                if (dist < minDistance)
                {
                    minDistance = dist;
                    minObjectHeader = &compoundObjectHeader->objectHeader;
                    minGeometryHeader = geometryHeader;
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
        
        if (outlineObjectHeader != 0)
        {
            if (!hit || (outlineObjectHeader != minObjectHeader))
            {
                return RayMarchResult { ray, outlineObjectHeader->objectID, float4{ 1, 1, 1, 1 }, 0.f };
            }
        }
        
        if (hit)
        {
            using MyShaderEvaluator = ShadeEvaluator<TShader>;
            MyShaderEvaluator shadeEvaluator { ray, minDistance, pt, _shader, minObjectHeader->materialID };

            const float4 color = evaluateTransformedGeometry<MyShaderEvaluator, float4>(shadeEvaluator, minGeometryHeader);
            
            return RayMarchResult { ray, minObjectHeader->objectID, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
