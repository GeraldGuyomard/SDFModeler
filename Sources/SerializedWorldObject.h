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
    uint16_t nbCommands = 0; // 2
    TDrawCommandIndex rootCommandIndex = -1; // 2
};


static CONSTANT constexpr size_t kMaxTiles = 64 * 64;
static CONSTANT constexpr size_t kPrimitivesBufferSize = 128 * kNbObjectsMax;
static CONSTANT constexpr size_t kDrawCommandArraySize = kMaxTiles * kNbObjectsMax;

static CONSTANT constexpr TDrawCommandIndex kInvalidCommandIndex(-1);

struct SerializedWorldObject final
{
    // environment
    SDFObject<SDFPlane> grid = { SDFPlane{} };

    //
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

struct Locals // 16
{
    float2 distances; // 8
    char2 relativeMinDrawCommandIndices; // 2
    
    uint8_t relativeDrawCommandIndex; // 1
    int8_t nbChildrenLeft; // 1 if < 0 it's a leaf primitive
    uint8_t isCulled; // 1
};

class CullingInfo final
{
public:
    CullingInfo() = default;
    
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
    
    CullingInfo subCulling(TDrawCommandIndex rootCommandIndex, TDrawCommandIndex startCommandIndex) const
    {
        const auto nbCmdsToSkip = startCommandIndex - rootCommandIndex;
        ASSERT(nbCmdsToSkip >= 0);
        
        return { _bits >> nbCmdsToSkip };
    }
    
private:
    CullingInfo(uint64_t bits)
    : _bits(bits)
    {}
    
    uint64_t _bits = 0;
};

class Stack final
{
public:
    Stack(CONSTANT SerializedWorldObject& serialized, TDrawCommandIndex rootCommandIndex, CullingInfo cullingInfo)
    : _serialized(serialized), _rootCommandIndex(rootCommandIndex), _cullingInfo(cullingInfo)
    {
        static_assert(sizeof(Locals) <= 16, "Locals became too big");
    }
    
    bool empty() const
    {
        return _stackIndex < 0;
    }
    
    int8_t depth() const
    {
        return _stackIndex;
    }
    
    void push()
    {
        ASSERT(_stackIndex < int8_t(kMaxStackDepth - 1));
        
        const auto relativeCommandIndex = _relativeCmdIndex++;
        CONSTANT DrawCommand* cmd = _serialized.drawCommand(_rootCommandIndex + relativeCommandIndex);
        
        THREAD auto& locals = _stack[++_stackIndex];
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount < 0)
        {
             locals.nbChildrenLeft = -cmd->primitiveOffsetOrNegativeChildrenCount;
        }
        else
        {
            locals.nbChildrenLeft = -100;
        }
        
        locals.relativeMinDrawCommandIndices = { -1, -1 };
        locals.relativeDrawCommandIndex = relativeCommandIndex;
        locals.distances = { 1e7f, 1e7f };
        locals.isCulled = _cullingInfo.nextCulling();
    }

    THREAD Locals* parentLocals()
    {
        if (_stackIndex >= 1)
        {
            return &_stack[_stackIndex - 1];
        }
        else
        {
            return nullptr;
        }
    }
    
    THREAD Locals& parentLocalsNoCheck()
    {
        ASSERT(_stackIndex >= 1);
        return _stack[_stackIndex - 1];
    }
    
    THREAD Locals& locals()
    {
        ASSERT(_stackIndex >= 0);
        return _stack[_stackIndex];
    }
    
    CONSTANT DrawCommand* currentDrawCommand() const
    {
        const TDrawCommandIndex index = _rootCommandIndex + _stack[_stackIndex].relativeDrawCommandIndex;
        return _serialized.drawCommand(index);
    }
    
    void back()
    {
        if (--_stackIndex >= 0)
        {
            --_stack[_stackIndex].nbChildrenLeft;
        }
    }
    
private:
    static CONSTANT constexpr size_t kMaxStackDepth = 8;
    
    CullingInfo _cullingInfo;
    CONSTANT SerializedWorldObject& _serialized;
    const TDrawCommandIndex _rootCommandIndex;
    Locals _stack[kMaxStackDepth];
    int8_t _stackIndex = -1;
    uint8_t _relativeCmdIndex = 0;
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
    
    TDrawCommandIndex minCmdIndex() const
    {
        return _minCmdIndex;
    }
    
    void setMinCmdIndex(TDrawCommandIndex index)
    {
        _minCmdIndex = index;
    }
    
    CullingInfo cullingInfo()
    {
        return _cullingInfo;
    }
    
    bool submitMinDistance(CONSTANT SerializedWorldObject& serialized, float dist)
    {
        if (dist < _minDistance)
        {
            _minDistance = dist;
            
            if (_minDistance <= kDistanceEpsilon)
            {
                _hit = true;
                return true;
            }
        }
        
        return false;
    }
    
private:
    CullingInfo _cullingInfo;
    
    float _minDistance = 1e5f;
    float _prevMinDistance = 1e5f;
    TDrawCommandIndex _minCmdIndex = -1;
    
    bool _hit = false;
};

INLINE void _computeDistIterative(
                           DistanceEvaluator distanceEvaluator,
                           THREAD Visitor& visitor,
                           CONSTANT SerializedWorldObject& serialized,
                           TDrawCommandIndex rootCommandIndex)
{
    Stack stack { serialized, rootCommandIndex, visitor.cullingInfo() };
    
    stack.push();
    
    while (!stack.empty())
    {
        THREAD auto& locals = stack.locals();
        const auto n = locals.nbChildrenLeft;
        
        if (n < 0)
        {
            // Leaf Primitive
            if (!locals.isCulled)
            {
                CONSTANT auto* cmd = stack.currentDrawCommand();
                auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                const float d = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
                const size_t opIndex = size_t(prim->sdfOperation());
                
                if (auto parentLocals = stack.parentLocals())
                {
                    if (d < parentLocals->distances[opIndex])
                    {
                        parentLocals->distances[opIndex] = d;
                        parentLocals->relativeMinDrawCommandIndices[opIndex] = locals.relativeDrawCommandIndex;
                    }
                }
                else if (prim->sdfOperation() == SDFOperation::addition)
                {
                    locals.distances[opIndex] = d;
                    visitor.submitMinDistance(serialized, d);
                }
            }

            stack.back();
        }
        else if (n > 0)
        {
            // groups could be culled too in future
            ASSERT(!locals.isCulled);
            
            // A inner Draw Command
            stack.push();
        }
        else
        {
            // == 0
            // -> a group and we just finished to go through the children
            const float2 distances = locals.distances;
            const float additiveObjectsDist = distances.x;
            const float negativeObjectsDist = -distances.y;
            const int relativeMinCmdIndex = additiveObjectsDist < negativeObjectsDist;
            
            const float dist = max(additiveObjectsDist, negativeObjectsDist);
            
            if (visitor.submitMinDistance(serialized, dist))
            {
                // hit
                const auto relativeCmdIndex = locals.relativeMinDrawCommandIndices[relativeMinCmdIndex];
                ASSERT(relativeCmdIndex >= 0);
                
                visitor.setMinCmdIndex(rootCommandIndex + relativeCmdIndex);
                
                break;
            }
            
            if (stack.depth() > 0)
            {
                THREAD auto& parentLocals = stack.parentLocalsNoCheck();
                parentLocals.distances[0] = min(parentLocals.distances[0], dist);
            }

            stack.back();
        }
    }
}

template <typename TVisitor>
void _visitDrawCommandTree(float3 pt, CONSTANT SerializedWorldObject& serialized, TDrawCommandIndex rootCmdIndex, THREAD TVisitor& visitor)
{
    DistanceEvaluator distanceEvaluator { pt };
    _computeDistIterative(distanceEvaluator, visitor, serialized, rootCmdIndex);
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
            visitor.submitMinDistance(serialized, dist);
            
            return dist;
        }
    }

#define CPU_ITERATIVE 1

INLINE void visitDrawCommandTree(float3 pt,
                    CONSTANT SerializedWorldObject& serialized,
                    TDrawCommandIndex rootCmdIndex,
                    THREAD Visitor& visitor)
{
#if CPU_ITERATIVE
    return _visitDrawCommandTree(pt, serialized, rootCmdIndex, visitor);
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

INLINE void visitFlatCommandList(float3 pt,
                    CONSTANT SerializedWorldObject& serialized,
                    TDrawCommandIndex rootCmdIndex,
                    size_t nbCommands,
                    THREAD Visitor& visitor)
{
    auto cmd = &serialized.drawCommands[rootCmdIndex];
    const auto end = cmd + nbCommands;
    
    auto cullingInfo = visitor.cullingInfo();
    
    DistanceEvaluator distanceEvaluator { pt };
    
    for (; cmd < end; ++cmd)
    {
        if (cullingInfo.nextCulling())
        {
            continue;
        }
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount >= 0)
        {
            auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
            const float d = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
            if (visitor.submitMinDistance(serialized, d))
            {
                visitor.setMinCmdIndex(serialized.drawCommandIndex(cmd));
                break;
            }
        }
    }
}

class ShadedPrimitive final
{
public:
    ShadedPrimitive(CONSTANT SerializedWorldObject& serialized,
                    TDrawCommandIndex rootCommandIndex,
                    MaterialID materialID,
                    CullingInfo cullingInfo)
    : _serialized(serialized),
    _rootCommandIndex(rootCommandIndex),
    _materialID(materialID),
    _cullingInfo(cullingInfo)
    {}
    
    MaterialID materialID() const
    {
        return _materialID;
    }
    
    float computeDistance(float3 pt) const
    {
        Visitor v;
        v.reset(_cullingInfo);
        visitDrawCommandTree(pt, _serialized, _rootCommandIndex, v);
        
        return v.minDistance();
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    const CullingInfo _cullingInfo;
    const MaterialID _materialID;
    
    TDrawCommandIndex _rootCommandIndex;
};

template <typename TShader>
RayMarchResult
computeHitResult(const THREAD Visitor& visitor,
                 CONSTANT SerializedWorldObject& serialized,
                 CONSTANT Tile& tile,
                 CullingInfo cullingInfo,
                 THREAD const Ray& ray,
                 float3 pt,
                 float d,
                 THREAD const TShader& shader)
{
    const auto minCmdIndex = visitor.minCmdIndex();
    ASSERT(minCmdIndex >= 0);
    
    // This should be a leaf primitive
    auto cmd = serialized.drawCommand(minCmdIndex);
    ASSERT(cmd->primitiveOffsetOrNegativeChildrenCount >= 0);
    
    const TDrawCommandIndex startCmdIndex = minCmdIndex + cmd->ownerOffset;
    
    auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
    const auto materialID = prim->materialId;
    const auto subCullingInfo = cullingInfo.subCulling(tile.rootCommandIndex, startCmdIndex);
    ShadedPrimitive primitive { serialized, startCmdIndex, materialID, subCullingInfo };
    const auto color = shader.computeShade(primitive, ray, visitor.minDistance(), pt);
    
    const auto objectID = prim->objectId;
    
    return RayMarchResult { ray, objectID, color, d };
}

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
        
        size_t nbObjectsPerOperation[2];
        nbObjectsPerOperation[0] = nbObjectsPerOperation[1] = 0;
        
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
                else
                {
                    ++nbObjectsPerOperation[prim->operation];
                }
            }
            
            cmd++;
        }
        
        if (nbObjectsPerOperation[size_t(SDFOperation::addition)] == 0)
        {
            return RayMarchResult { ray };
        }
        
        constexpr size_t kNbSteps = 100;
        
        float d = 0.f;
        float3 pt = ray.origin;
        Visitor visitor;
        
        for (size_t i=0; i < kNbSteps; ++i)
        {
            pt = ray.pt(d);
            
            visitor.reset(cullingInfo);
            
            if (nbObjectsPerOperation[size_t(SDFOperation::substraction)] != 0)
            {
                visitDrawCommandTree(pt, _serialized, tile.rootCommandIndex, visitor);
            }
            else
            {
                // only positive objects, can ignore the tree structure and iterate flat
                visitFlatCommandList(pt, _serialized, tile.rootCommandIndex, tile.nbCommands, visitor);
            }
            
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
        
        if (visitor.hit())
        {
            return computeHitResult(visitor, _serialized, tile, cullingInfo, ray, pt, d, _shader);
        }
        
        return RayMarchResult { ray };
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    TShader _shader;
};
