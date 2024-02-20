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
    TDrawCommandIndex rootCommandIndex = -1; // 8
};


static CONSTANT constexpr size_t kMaxTiles = 16 * 16;
static CONSTANT constexpr size_t kPrimitivesBufferSize = 128 * kNbObjectsMax;
static CONSTANT constexpr size_t kDrawCommandArraySize = kMaxTiles * kNbObjectsMax;

static CONSTANT constexpr TDrawCommandIndex kInvalidCommandIndex(-1);

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
        return reinterpret_cast<CONSTANT EncodedPrimitive*>(primitivesBuffer + offset);
    }
    
    TDrawCommandIndex drawCommandIndex(CONSTANT DrawCommand* cmd) CONSTANT
    {
        return TDrawCommandIndex(cmd - drawCommands);
    }
    
    CONSTANT DrawCommand* drawCommand(TDrawCommandIndex index) CONSTANT
    {
        return &drawCommands[index];
    }
};


class Stack final
{
public:
    Stack() = default;
    
    bool empty() const
    {
        return _stackIndex < 0;
    }
    
    int8_t depth() const
    {
        return _stackIndex;
    }
    
    void push(CONSTANT SerializedWorldObject& serialized, CONSTANT DrawCommand* cmd)
    {
        ++_stackIndex;
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount < 0)
        {
            _nbChildrenLeft[_stackIndex] = -cmd->primitiveOffsetOrNegativeChildrenCount;
        }
        else
        {
            _nbChildrenLeft[_stackIndex] = 0;
        }
        
        _drawCommandIndex[_stackIndex] = serialized.drawCommandIndex(cmd);
        _distances[_stackIndex] = { 1e7f, 1e7f };
    }
    
    TDrawCommandIndex drawCommandIndex() const
    {
        return _drawCommandIndex[_stackIndex];
    }
    
    float2 distances() const
    {
        return _distances[_stackIndex];
    }
    
    float2 parentDistances()
    {
        return _distances[_stackIndex - 1];
    }
    
    void setParentDistances(float2 d)
    {
        _distances[_stackIndex - 1] = d;
    }
    
    int8_t nbChildrenLeft() const
    {
        return _nbChildrenLeft[_stackIndex];
    }
    
    void oneLessChildrenLeft()
    {
        --_nbChildrenLeft[_stackIndex];
    }
    
    void back()
    {
        --_stackIndex;
    }
    
private:
    static CONSTANT constexpr size_t kMaxStackDepth = 7;
    
    float2 _distances[kMaxStackDepth];
    TDrawCommandIndex _drawCommandIndex[kMaxStackDepth];
    int8_t _nbChildrenLeft[kMaxStackDepth];
    
    int8_t _stackIndex = -1;
};

template <typename TVisitor>
void _computeDistIterative(
                           DistanceEvaluator distanceEvaluator,
                           THREAD TVisitor& visitor,
                           CONSTANT SerializedWorldObject& serialized,
                           CONSTANT DrawCommand*& inCmd)
{
    if (visitor.nextCulling())
    {
        return;
    }
    
    Stack stack;
    stack.push(serialized, inCmd++);
    
    while (!stack.empty())
    {
        CONSTANT auto* cmd = serialized.drawCommand(stack.drawCommandIndex());
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount >= 0)
        {
            auto parentDistances = stack.parentDistances();

            auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
            const float d = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
            const size_t opIndex = size_t(prim->sdfOperation());
            parentDistances[opIndex] = min(parentDistances[opIndex], d);
            
            stack.setParentDistances(parentDistances);
            
            stack.back();
        }
        else
        {
            auto n = stack.nbChildrenLeft();
            if (n > 0)
            {
                stack.oneLessChildrenLeft();
                
                CONSTANT auto* childCmd = ++inCmd;
                if (!visitor.nextCulling())
                {
                    stack.push(serialized, childCmd);
                }
            }
            else
            {
                const float2 distances = stack.distances();
                const float dist = max(distances.x, -distances.y);
                visitor.submitMinDistance(serialized, dist, cmd);
                
                if (stack.depth() > 0)
                {
                    float2 parentDistances = stack.parentDistances();
                    
                    parentDistances[0] = min(parentDistances[0], dist);
                    
                    stack.setParentDistances(parentDistances);
                }
                
                stack.back();
            }
        }
    }
}

template <typename TVisitor>
void _visitDrawCommandTree(float3 pt, CONSTANT SerializedWorldObject& serialized, TDrawCommandIndex rootCmdIndex, THREAD TVisitor& visitor)
{
    CONSTANT DrawCommand* cmd = serialized.drawCommand(rootCmdIndex);
    DistanceEvaluator distanceEvaluator { pt };
    
    _computeDistIterative<TVisitor>(distanceEvaluator, visitor, serialized, cmd);
}

#if SHADER_ON_CPU
    
    template <typename TVisitor>
    float computeDistRecursive(DistanceEvaluator distanceEvaluator,
                               THREAD TVisitor& visitor,
                               CONSTANT SerializedWorldObject& serialized,
                               CONSTANT DrawCommand*& inCmd)
    {
        auto cmd = inCmd++;
        
        if (visitor.nextCulling())
        {
            return 1e7f;
        }
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount >= 0)
        {
            auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
            const float dist = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
            return dist;
        }
        else
        {
            int32_t nbChildrenLeft = -cmd->primitiveOffsetOrNegativeChildrenCount;
            float2 distances { 1e7f, 1e7f };
            
            while (nbChildrenLeft > 0)
            {
                auto childCmd = inCmd;
                const float childDist = computeDistRecursive(distanceEvaluator, visitor, serialized, inCmd);
                if (childCmd->primitiveOffsetOrNegativeChildrenCount < 0)
                {
                    distances.x = min(distances.x, childDist);
                }
                else
                {
                    auto childPrim = serialized.primitive(childCmd->primitiveOffsetOrNegativeChildrenCount);
                    const size_t op = size_t(childPrim->sdfOperation());
                    distances[op] = min(distances[op], childDist);
                }
                
                --nbChildrenLeft;
            }
            
            const float dist = max(distances.x, -distances.y);
            visitor.submitMinDistance(serialized, dist, cmd);
            
            return dist;
        }
    }

#define CPU_ITERATIVE 1

template <typename TVisitor>
void visitDrawCommandTree(float3 pt,
                    CONSTANT SerializedWorldObject& serialized,
                    TDrawCommandIndex rootCmdIndex,
                    THREAD TVisitor& visitor)
{
#if CPU_ITERATIVE
    return _visitDrawCommandTree<TVisitor>(pt, serialized, rootCmdIndex, visitor);
#else
    CONSTANT DrawCommand* cmd = serialized.drawCommand(rootCmdIndex);
    DistanceEvaluator distanceEvaluator { pt };
    computeDistRecursive<TVisitor>(distanceEvaluator, visitor, serialized, cmd);
#endif
}

#else

// GPU
template <typename TVisitor>
void visitDrawCommandTree(float3 pt,
                    CONSTANT SerializedWorldObject& serialized,
                    TDrawCommandIndex rootCmdIndex,
                    THREAD TVisitor& visitor)
{
    _visitDrawCommandTree<TVisitor>(pt, serialized, rootCmdIndex, visitor);
}

#endif


class CullingInfo final
{
public:
    void storeCulling(size_t bitIndex)
    {
        _bits |= (1 << bitIndex);
    }
    
    bool nextCulling()
    {
        const bool culled = (_bits & 1);
        _bits >>= 1;
        
        return culled;
    }
    
private:
    uint64_t _bits = 0;
};

class Visitor final
{
public:
    Visitor() = default;
    
    void reset(CullingInfo info)
    {
        _cullingInfo = info;
        _prevMinDistance = _minDistance;
        _minDistance = 1e5f;
        _minCmdIndex = -1;
    }
    
    float minDistance() const
    {
        return _minDistance;
    }
    
    bool hit() const
    {
        return _hit;
    }
    
    TPrimitiveOffset outlinePrimOffset(CONSTANT SerializedWorldObject& serialized) const
    {
        if (_outlineCmdIndex >= 0)
        {
            const TPrimitiveOffset offset = serialized.drawCommand(_outlineCmdIndex)->primitiveOffsetOrNegativeChildrenCount;
            ASSERT(offset >= 0);
            return offset;
        }
        else
        {
            return kInvalidPrimitiveOffset;
        }
    }
    
    TDrawCommandIndex minCmdIndex() const
    {
        return _minCmdIndex;
    }
    
    bool nextCulling()
    {
        return _cullingInfo.nextCulling();
    }
    
    void submitMinDistance(CONSTANT SerializedWorldObject& serialized, float dist, CONSTANT DrawCommand* cmd)
    {
        if (dist < _minDistance)
        {
            _minDistance = dist;
            
            const auto index = serialized.drawCommandIndex(cmd);
            if (index != 0)
            {
                _minCmdIndex = index;
            }
            
            if (_minDistance <= kDistanceEpsilon)
            {
                _hit = true;
            }
        }
    }
    
private:
    CullingInfo _cullingInfo;
    
    uint8_t _currentDepth = 0;
    float _minDistance = 1e5f;
    float _prevMinDistance = 1e5f;
    TDrawCommandIndex _minCmdIndex = -1;
    TDrawCommandIndex _outlineCmdIndex = -1;
    
    bool _hit = false;
};

class ShadedPrimitive final
{
public:
    ShadedPrimitive(CONSTANT SerializedWorldObject& serialized, TDrawCommandIndex rootCommandIndex)
    : _serialized(serialized), _rootCommandIndex(rootCommandIndex)
    {}
    
    MaterialID materialID() const
    {
        auto cmd = _serialized.drawCommand(_rootCommandIndex);
        return cmd->materialID;
    }
    
    float computeDistance(float3 pt) const
    {
        Visitor v;
        visitDrawCommandTree(pt, _serialized, _rootCommandIndex, v);
        
        return v.minDistance();
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    TDrawCommandIndex _rootCommandIndex;
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
        
        CullEvaluator cullEvaluator { ray };
        
        CullingInfo cullingInfo;
        
        auto cmd = _serialized.drawCommand(tile.rootCommandIndex);
        for (uint8_t i=0; i < tile.nbCommands; ++i)
        {
            if (cmd->primitiveOffsetOrNegativeChildrenCount >= 0)
            {
                auto prim = _serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, prim);
                if (culled)
                {
                    cullingInfo.storeCulling(i);
                }
            }
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        
        float3 pt = ray.origin;
        
        Visitor visitor;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            visitor.reset(cullingInfo);
            
            visitDrawCommandTree<Visitor>(pt, _serialized, tile.rootCommandIndex, visitor);
            
            if (visitor.hit())
            {
                break;
            }
            
            d += visitor.minDistance();
            
            if (d > ray.maxLength)
            {
                break;
            }
        }
        
        const auto outlinePrimOffset = visitor.outlinePrimOffset(_serialized);
        
        /*if (outlinePrimOffset >= 0)
        {
            if (!visitor.hit())
            {
                CONSTANT EncodedPrimitive* minHeader = _serialized.primitive(outlinePrimOffset);
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
        }*/
        
        if (visitor.hit())
        {
            const auto minCmdIndex = visitor.minCmdIndex();
            
            float4 color;
            ObjectID objectID;
            
            if (minCmdIndex >= 0)
            {
                //ASSERT(minCmdIndex >= 0);
                
                ShadedPrimitive primitive { _serialized, minCmdIndex };
                color = _shader.computeShade(primitive, ray, visitor.minDistance(), pt);
                objectID = cmd->objectID;
            }
            else
            {
                color = { 1, 1, 1, 1 };
                objectID = 1;
            }
            
            
            return RayMarchResult { ray, objectID, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    TShader _shader;
};
