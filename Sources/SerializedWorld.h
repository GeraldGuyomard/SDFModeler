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
        
        CONSTANT uint8_t* buffer = &_serializedWorld.geometries[0];
        CONSTANT TransformedGeometryHeader* geometryHeader = reinterpret_cast<CONSTANT TransformedGeometryHeader*>(buffer);
        
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
        
        for (size_t i=0; i < _serializedWorld.compoundObjectsCount; ++i)
        {
            /*CONSTANT SimpleObjectHeader* objectHeader = &_serializedWorld.simpleObjectHeaders[i];
            if (!geometryCulled[objectHeader->geometryIndex])
            {
                simpleObjectHeaders[simpleObjectsCount++] = objectHeader;
            }*/
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        uint32_t outlineObjectID = 0;
        bool hit = false;
        
        float minDistance = 1e5f;
        float prevMinDistance = minDistance;
        
        float3 pt = ray.origin;
        CONSTANT SimpleObjectHeader* minObjectHeader = nullptr;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            minDistance = 1e5f;
            minObjectHeader = nullptr;
            DistanceEvaluator distanceEvaluator { pt };
            
            for (size_t objectIndex = 0; objectIndex < simpleObjectsCount; ++objectIndex)
            {
                CONSTANT SimpleObjectHeader* simpleObjectHeader = simpleObjectHeaders[objectIndex];
                geometryHeader = geometryHeaders[simpleObjectHeader->geometryIndex];
                
                const float dist = evaluateTransformedGeometry<DistanceEvaluator, float>(distanceEvaluator, geometryHeader);
                
                if ((outlineObjectID == 0) && simpleObjectHeader->selected)
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineObjectID = simpleObjectHeader->objectID;
                    }
                }
                
                if (dist < minDistance)
                {
                    minDistance = dist;
                    minObjectHeader = simpleObjectHeader;
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
        
        if (outlineObjectID != 0)
        {
            if (!hit || (outlineObjectID != minObjectHeader->objectID))
            {
                return RayMarchResult { ray, outlineObjectID, float4{ 1, 1, 1, 1 }, 0.f };
            }
        }
        
        if (hit)
        {
            using MyShaderEvaluator = ShadeEvaluator<TShader>;
            MyShaderEvaluator shadeEvaluator { ray, minDistance, pt, _shader, minObjectHeader->materialID };
            CONSTANT TransformedGeometryHeader* geomHeader = geometryHeaders[minObjectHeader->geometryIndex];
            const float4 color = evaluateTransformedGeometry<MyShaderEvaluator, float4>(shadeEvaluator, geomHeader);
            
            return RayMarchResult { ray, minObjectHeader->objectID, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
