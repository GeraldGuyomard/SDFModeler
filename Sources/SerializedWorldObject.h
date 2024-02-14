//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Uniforms.h"
#include "Results.h"
#include "PrimitiveEvaluator.h"

#include "ShadedPrimitive.h"

struct Tile final
{
    float2 minPt = { 0, 0 }; // 8
    float2 maxPt = { 0, 0 }; // 8
    size_t nbCommands = 0; // 8
    size_t rootCommandIndex = -1; // 8
};

struct DrawCommand final
{
    size_t nbPositiveChildren = 0;
    size_t nbNegativeChildren = 0;
    
    TPrimitiveOffset primitiveOffset = 0; // -1 if no primitive
};

static CONSTANT constexpr size_t kMaxTiles = 16 * 16;
static CONSTANT constexpr size_t kPrimitivesBufferSize = 128 * kNbObjectsMax;
static CONSTANT constexpr size_t kDrawCommandArraySize = kMaxTiles * kNbObjectsMax;

static CONSTANT constexpr size_t kInvalidCommandIndex = size_t(-1);

struct SerializedWorldObject final
{
    float2 tileSize = { 0.f, 0.f };
    float  numTileColumns = 0;
    float  numTileRows = 0;
    
    Tile tiles[kMaxTiles];
    
    // should be aligned on 16 bytes
    // for SSE float moves
    
    // buffer is an array of serialized objects
    // that starts with ObjectHeaders
    uint8_t primitivesBuffer[kPrimitivesBufferSize];
    
    DrawCommand drawCommands[kDrawCommandArraySize];
    
    CONSTANT EncodedPrimitive* primitive(TPrimitiveOffset offset) CONSTANT
    {
        if (offset == kInvalidPrimitiveOffset)
        {
            return nullptr;
        }
        else
        {
            return reinterpret_cast<CONSTANT EncodedPrimitive*>(primitivesBuffer + offset);
        }
    }
};

class DrawCommandStack final
{
public:
    DrawCommandStack(CONSTANT Tile& tile, CONSTANT SerializedWorldObject& serializedWorldObject)
    : _serializedWorldObject(serializedWorldObject),
    _nbCommands(tile.nbCommands),
    _rootCommandIndex(tile.rootCommandIndex)
    {
    }
    
    CONSTANT EncodedPrimitive* primitive() const
    {
        return _serializedWorldObject.primitive(current()->primitiveOffset);
    }
    
    size_t nbPositiveChildren() const
    {
        return current()->nbPositiveChildren;
    }

    size_t nbNegativeChildren() const
    {
        return current()->nbNegativeChildren;
    }
    
    CONSTANT DrawCommand* next()
    {
        ++_relativeIndex;
        return current();
    }
    
    void enterChildren()
    {
        _indexStack[_stackSize++] = _relativeIndex++;
    }
    
    void exitChildren()
    {
        --_stackSize;
        _relativeIndex = _indexStack[_stackSize];
    }
    
private:
    
    CONSTANT DrawCommand* current() const
    {
        if (_relativeIndex == _nbCommands)
        {
            return nullptr;
        }
        
        return reinterpret_cast<CONSTANT DrawCommand*>(_serializedWorldObject.drawCommands + _rootCommandIndex + _relativeIndex);
    }
    
    CONSTANT SerializedWorldObject& _serializedWorldObject;
    
    static CONSTANT constexpr size_t kMaxStackSize = 8;
    const int64_t _nbCommands;
    const int64_t _rootCommandIndex;
    
    size_t _indexStack[kMaxStackSize];
    size_t _stackSize = 0;
    int64_t _relativeIndex = -1;
};


template <typename TShader>
class WorldObject final
{
public:
    
    WorldObject(TShader shader, CONSTANT SerializedWorldObject& serializedWorld)
    : _serialized(serializedWorld), _shader(shader)
    {}
    
    RayMarchResult rayMarch(float2 ndcPosition, float2 viewportSize, Ray ray) const
    {
        // position between 0 and width, height on both axis
        float2 position;
        position.x = (ndcPosition.x + 1.f) * 0.5f;
        position.y = (-ndcPosition.y + 1.f) * 0.5f;
        position *= viewportSize;
        
        float2 tileCoordinates = position / _serialized.tileSize;
        tileCoordinates = floor(tileCoordinates);
        
        const size_t tileIndex = (tileCoordinates.y * _serialized.numTileColumns) + tileCoordinates.x;
        //const size_t tileIndex = 0;
        
        CONSTANT Tile& tile = _serialized.tiles[tileIndex];
        
        if (tile.rootCommandIndex == kInvalidCommandIndex)
        {
            return RayMarchResult { ray };
        }
        
        EncodedPrimitiveArray primsArray { &_serialized.primitivesBuffer[0] };
        
        CullEvaluator cullEvaluator { ray };
        
        bool hasNegativePrims = false;
        
        DrawCommandStack stack { tile, _serialized };
        
        while (auto cmd = stack.next())
        {
            bool hasPositivePrims = false;
            
            if (auto prim = stack.primitive())
            {
                // primitive at this level
                hasPositivePrims = true;
                primsArray.add(prim);
            }
            
            const size_t nbPositiveChildren = stack.nbPositiveChildren();
            const size_t nbNegativeChildren = stack.nbNegativeChildren();
            
            stack.enterChildren();
            
            for (size_t i=0; i < nbPositiveChildren; ++i)
            {
                auto childPrim = stack.primitive();
                if (childPrim != nullptr)
                {
                    const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, childPrim);
                    if (!culled)
                    {
                        hasPositivePrims = true;
                        primsArray.add(childPrim);
                    }
                }
            }
            
            if (hasPositivePrims)
            {
                // cull all the negative parts
                for (size_t i=0; i < nbNegativeChildren; ++i)
                {
                    auto childPrim = stack.primitive();
                    const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, childPrim);
                    if (!culled)
                    {
                        primsArray.add(childPrim);
                        hasNegativePrims = true;
                    }
                }
            }
            
            stack.exitChildren();
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        int64_t outlinePrimIndex = -1;
        bool hit = false;
        
        float minDistance = 1e5f;
        float prevMinDistance = minDistance;
        
        float3 pt = ray.origin;
        int64_t minPrimIndex = -1;
        
        const size_t nbPrims = primsArray.nbPrimitives();
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            minDistance = 1e5f;
            minPrimIndex = -1;
            
            size_t primIndex = 0;
            
            while (primIndex < nbPrims)
            {
                const auto startIndex = primIndex;
                
                const float dist = primsArray.computeDistance(pt, primIndex);
                
                CONSTANT EncodedPrimitive* startPrim = primsArray.primitive(startIndex);
                
                if (startPrim->selected && (outlinePrimIndex < 0))
                {
                    if ((prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        outlinePrimIndex = startIndex;
                    }
                }
                
                if (dist <= minDistance)
                {
                    minDistance = dist;
                    minPrimIndex = startIndex;
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
        
        if (outlinePrimIndex >= 0)
        {
            if (!hit)
            {
                CONSTANT EncodedPrimitive* minHeader = primsArray.primitive(minPrimIndex);
                return RayMarchResult { ray, minHeader->objectId, float4{ 1, 1, 1, 1 }, 0.f };
            }
            else if (outlinePrimIndex != minPrimIndex)
            {
                CONSTANT EncodedPrimitive* outlinePrimitive = primsArray.primitive(outlinePrimIndex);
                CONSTANT EncodedPrimitive* minPrimitive = primsArray.primitive(minPrimIndex);
                
                if (outlinePrimitive->objectId != minPrimitive->objectId)
                {
                    return RayMarchResult { ray, minPrimitive->objectId, float4{ 1, 1, 1, 1 }, 0.f };
                }
            }
        }
        
        if (hit)
        {
            ShadedPrimitive primitive { primsArray, size_t(minPrimIndex) };
            const float4 color = _shader.computeShade(primitive, ray, minDistance, pt);
            
            CONSTANT EncodedPrimitive* minPrim = primsArray.primitive(minPrimIndex);
            
            return RayMarchResult { ray, minPrim->objectId, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    TShader _shader;
};
