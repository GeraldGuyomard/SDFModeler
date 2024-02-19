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

using TDrawCommandIndex = int16_t;
struct DrawCommand final
{
    TPrimitiveOffset primitiveOffsetOrNegativeChildrenCount = 0; // -1 if no primitive
};

static CONSTANT constexpr size_t kMaxTiles = 16 * 16;
static CONSTANT constexpr size_t kPrimitivesBufferSize = 128 * kNbObjectsMax;
static CONSTANT constexpr size_t kDrawCommandArraySize = kMaxTiles * kNbObjectsMax;

static CONSTANT constexpr size_t kInvalidCommandIndex(-1);

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

class DrawCommandStack final
{
public:
    DrawCommandStack(CONSTANT Tile& tile, CONSTANT SerializedWorldObject& serializedWorldObject)
    : _serializedWorldObject(serializedWorldObject),
    _nbCommands(tile.nbCommands),
    _rootCommandIndex(tile.rootCommandIndex)
    {
    }
    
    TPrimitiveOffset primitiveOffset() const
    {
        return current()->primitiveOffsetOrNegativeChildrenCount;
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
    
    static CONSTANT constexpr size_t kMaxStackSize = 4;
    const int64_t _nbCommands;
    const int64_t _rootCommandIndex;
    
    uint8_t _indexStack[kMaxStackSize];
    uint8_t _stackSize = 0;
    int8_t _relativeIndex = -1;
};

template <typename TLocals>
class Stack final
{
public:
    Stack() = default;
    
    bool empty() const { return _stackIndex < 0; }
    void push(THREAD const TLocals& locals)
    {
        _stack[++_stackIndex] = locals;
    }
    
    THREAD TLocals& current()
    {
        ASSERT(_stackIndex >= 0);
        return _stack[_stackIndex];
    }
    
    void pop()
    {
        ASSERT(_stackIndex > 0);
        --_stackIndex;
    }
    
private:
    static CONSTANT constexpr size_t kMaxStackDepth = 7;
    TLocals _stack[kMaxStackDepth];
    int8_t _stackIndex = -1;
};

template <typename TVisitor, typename TLocals>
void visitDrawCommandTree(CONSTANT SerializedWorldObject& serialized, TDrawCommandIndex rootCmdIndex, THREAD TVisitor& visitor)
{
    auto cmd = &serialized.drawCommands[rootCmdIndex];
    Stack<TLocals> stack;
    stack.push(visitor.locals(serialized, cmd));
    
    while (!stack.empty())
    {
        auto& locals = stack.current();
        visitor.visit(serialized, cmd, locals);
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount < 0)
        {
            const size_t n = -cmd->primitiveOffsetOrNegativeChildrenCount;
            for (size_t i=0; i < n; ++i)
            {
                ++cmd;
                stack.push(visitor.locals(serialized, cmd));
            }
        }
        
        stack.pop();
    }
}

class CullingInfo final
{
public:
    void storeCulling(bool culled)
    {
        _bits = (_bits << 1) | (culled? 1 : 0);
    }
    
    void storeNoCulling()
    {
        _bits <<= 1;
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

class Locals
{
public:
    Locals() = default;
    
    float distance;
};

class Visitor final
{
public:
    Visitor(float3 pt)
    : _pt(pt)
    {}
    
    void reset(CullingInfo info)
    {
        _cullingInfo = info;
        _prevMinDistance = _minDistance;
        _minDistance = 1e5f;
        _minCmdIndex = -1;
    }
    
    Locals locals(CONSTANT SerializedWorldObject& serialized, CONSTANT DrawCommand*) const
    {
        return {};
    }
    
    void visit(CONSTANT SerializedWorldObject& serialized, CONSTANT DrawCommand* cmd, THREAD Locals& locals)
    {
        locals.distance = 1e7f;
        
        const bool culled = _cullingInfo.nextCulling();
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount >= 0)
        {
            if (!culled)
            {
                auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                const float dist = ::computeDistance(_pt, prim);
                
                locals.distance = dist;
                
                if (prim->selected && (_outlineCmdIndex < 0))
                {
                    if ((_prevMinDistance < dist) && (dist < kOutlineThickness))
                    {
                        _outlineCmdIndex = serialized.drawCommandIndex(cmd);
                    }
                }
                
                if (dist <= _minDistance)
                {
                    _minDistance = dist;
                    _outlineCmdIndex = serialized.drawCommandIndex(cmd);
                }
            }
        }
        
        if (locals.distance <= kDistanceEpsilon)
        {
            _hit = true;
        }
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
    
private:
    const float3 _pt;
    CullingInfo _cullingInfo;
    
    float _minDistance = 1e5f;
    float _prevMinDistance = 1e5f;
    TDrawCommandIndex _minCmdIndex = -1;
    TDrawCommandIndex _outlineCmdIndex = -1;
    
    bool _hit = false;
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
        const auto end = cmd + tile.nbCommands;
        for (;cmd < end; ++cmd)
        {
            if (cmd->primitiveOffsetOrNegativeChildrenCount >= 0)
            {
                auto prim = _serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                const bool culled = evaluatePrimitive<CullEvaluator, bool>(cullEvaluator, prim);

                if (!culled)
                {
                    int a;
                    a = 1;
                }
                
                cullingInfo.storeCulling(culled);
            }
            else
            {
                cullingInfo.storeNoCulling();
            }
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        
        float3 pt = ray.origin;
        
        Visitor visitor { pt };
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            visitor.reset(cullingInfo);
            
            visitDrawCommandTree<Visitor, Locals>(_serialized, tile.rootCommandIndex, visitor);
            
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
            ASSERT(minCmdIndex >= 0);
            
            auto cmd = _serialized.drawCommand(minCmdIndex);
            auto prim = _serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
            
            ShadedPrimitive primitive { prim };
            const float4 color = _shader.computeShade(primitive, ray, visitor.minDistance(), pt);
            
            return RayMarchResult { ray, prim->objectId, color, d };
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    TShader _shader;
};
