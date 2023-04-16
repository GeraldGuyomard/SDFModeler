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
        
        struct GeometryEntry
        {
            bool culled;
            CONSTANT TransformedGeometryHeader* header;
            
            GeometryEntry() = default;
            GeometryEntry(bool culled, CONSTANT TransformedGeometryHeader* header)
            : culled(culled), header(header)
            {}
        };
        GeometryEntry geometryEntries[kNbObjectsMax];
        
        CONSTANT uint8_t* buffer = &_serializedWorld.geometries[0];
        CONSTANT TransformedGeometryHeader* header = reinterpret_cast<CONSTANT TransformedGeometryHeader*>(buffer);
        
        for (size_t i=0; i < _serializedWorld.geometriesCount; ++i)
        {
            bool culled = evaluateTransformedGeometry<CullEvaluator, bool>(cullEvaluator, header);
            
            geometryEntries[i] = { culled, header };
            
            header = TransformedGeometryHeader::next(header);
        }
        
        size_t simpleObjectsCount = 0;
        
        struct SimpleObjectEntry final
        {
            uint32_t                    objectID;
            uint32_t                    materialID;
            const THREAD GeometryEntry* geometryEntry;
            bool                        selected;
            
            SimpleObjectEntry() = default;
            SimpleObjectEntry(CONSTANT SimpleObjectHeader& header, const THREAD GeometryEntry* geomEntry)
            : objectID(header.objectID),
            materialID(header.materialID),
            geometryEntry(geomEntry),
            selected(header.selected)
            {}
        };
        
        SimpleObjectEntry simpleObjectEntries[kNbObjectsMax];
        for (size_t i=0; i < _serializedWorld.simpleObjectsCount; ++i)
        {
            CONSTANT SimpleObjectHeader& sourceHeader = _serializedWorld.simpleObjectHeaders[i];
            const THREAD GeometryEntry* geomEntry = &geometryEntries[sourceHeader.geometryIndex];
            if (!geomEntry->culled)
            {
                simpleObjectEntries[simpleObjectsCount++] = { sourceHeader, geomEntry };
            }
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        THREAD const SimpleObjectEntry* outlineObjectEntry = nullptr;
        bool hit = false;
        
        float minDistance = 1e5f;
        float prevMinDistance = minDistance;
        
        float3 pt = ray.origin;
        THREAD const SimpleObjectEntry* minObjectEntry = nullptr;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            minDistance = 1e5f;
            minObjectEntry = nullptr;
            DistanceEvaluator distanceEvaluator { pt };
            
            for (size_t objectIndex = 0; objectIndex < simpleObjectsCount; ++objectIndex)
            {
                const THREAD auto* simpleObjectEntry = &simpleObjectEntries[objectIndex];
                CONSTANT TransformedGeometryHeader* header = simpleObjectEntry->geometryEntry->header;
                
                const float dist = evaluateTransformedGeometry<DistanceEvaluator, float>(distanceEvaluator, header);
                
                if ((outlineObjectEntry == nullptr) && simpleObjectEntry->selected)
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlineObjectEntry = simpleObjectEntry;
                    }
                }
                
                if (dist <= minDistance)
                {
                    minDistance = dist;
                    minObjectEntry = simpleObjectEntry;
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
        
        if (outlineObjectEntry != nullptr)
        {
            if (!hit || (outlineObjectEntry != minObjectEntry))
            {
                return RayMarchResult { ray, minObjectEntry->objectID, float4{ 1, 1, 1, 1 }, 0.f };
            }
        }
        
        if (hit)
        {
            using MyShaderEvaluator = ShadeEvaluator<TShader>;
            MyShaderEvaluator shadeEvaluator { ray, minDistance, pt, _shader, minObjectEntry->materialID };
            const float4 color = evaluateTransformedGeometry<MyShaderEvaluator, float4>(shadeEvaluator, minObjectEntry->geometryEntry->header);
            
            return RayMarchResult { ray, minObjectEntry->objectID, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorld& _serializedWorld;
    TShader _shader;
};
