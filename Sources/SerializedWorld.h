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
    uint32_t compositionCount = 0;
    
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
    
    constexpr static CONSTANT size_t kNbObjectsMax = 128;
    
    Content(TShader shader, CONSTANT SerializedWorld& serializedWorld)
    : _serializedWorld(serializedWorld), _shader(shader)
    {}
    
    RayMarchResult rayMarch(Ray ray) const
    {
        CullEvaluator cullEvaluator { ray };
        
        CONSTANT TransformedGeometryHeader* geometryHeaders[kNbObjectsMax];
        
        CONSTANT uint8_t* buffer = &_serializedWorld.geometries[0];
        CONSTANT TransformedGeometryHeader* header = reinterpret_cast<CONSTANT TransformedGeometryHeader*>(buffer);
        
        bool geometryCulled[20];
        
        for (size_t i=0; i < _serializedWorld.geometriesCount; ++i)
        {
            bool culled = evaluateTransformedGeometry<CullEvaluator, bool>(cullEvaluator, header);
            
            geometryHeaders[i] = header;
            geometryCulled[i] = culled;
            
            header = TransformedGeometryHeader::next(header);
        }
        
        size_t simpleObjectsCount = 0;
        
        CONSTANT SimpleObjectHeader* simpleObjectHeaders[kNbObjectsMax];
        for (size_t i=0; i < _serializedWorld.simpleObjectsCount; ++i)
        {
            CONSTANT SimpleObjectHeader* objectHeader = &_serializedWorld.simpleObjectHeaders[i];
            if (!geometryCulled[objectHeader->geometryIndex])
            {
                simpleObjectHeaders[simpleObjectsCount++] = objectHeader;
            }
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        CONSTANT SimpleObjectHeader* outlineObjectHeader = nullptr;
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
                header = geometryHeaders[simpleObjectHeader->geometryIndex];
                
                const float dist = evaluateTransformedGeometry<DistanceEvaluator, float>(distanceEvaluator, header);
                
                if ((outlineObjectHeader == nullptr) && simpleObjectHeader->selected)
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineObjectHeader = simpleObjectHeader;
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
        
        if (outlineObjectHeader != nullptr)
        {
            if (!hit || (outlineObjectHeader != minObjectHeader))
            {
                return RayMarchResult { ray, minObjectHeader->objectID, float4{ 1, 1, 1, 1 }, 0.f };
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
