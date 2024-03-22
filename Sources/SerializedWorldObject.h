//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CameraUniforms.h"
#include "Results.h"
#include "PrimitiveEvaluator.h"

#include "ShadedPrimitive.h"

struct Tile final
{
    float2 minPt = { 0, 0 }; // 8
    float2 maxPt = { 0, 0 }; // 8
    uint16_t nbCommands = 0; // 2
    TDrawCommandIndex rootCommandIndex = -1; // 2
    
    enum Flags
    {
        fHasBlendedPrimitives = 1 << 0
    };
    
    uint16_t flags = 0; // 4
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

struct Locals // 8
{
    float distance; // 4
    
    char relativeMinDrawCommandIndex; // 1
    uint8_t relativeDrawCommandIndex; // 1
    int8_t nbChildrenLeft; // 1 if < 0 it's a leaf primitive
    
    // 1
    
    enum FFlags
    {
        fIsCulled = 1 << 0,
        fIsMinDrawCommandSubstractive = 1 << 1
    };
    
    uint8_t flags;
    
    bool isCulled() const
    {
        return (flags & fIsCulled) != 0;
    }
    
    bool isMinDrawCommandSubstractive() const
    {
        return (flags & fIsMinDrawCommandSubstractive) != 0;
    }
    
    Locals()
    {
        static_assert(sizeof(Locals) == 8, "should simply be one 64 bit word");
    }
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
    
    bool nothingCulled() const
    {
        return _bits == 0;
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
    {}
    
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
        
        THREAD auto& locals = _stack[++_stackIndex];
        
        const auto relativeCommandIndex = _relativeCmdIndex++;
        
        const bool isCulled = _cullingInfo.nextCulling();
        locals.flags = isCulled ? Locals::FFlags::fIsCulled : 0;
        
        CONSTANT DrawCommand* cmd = _serialized.drawCommand(_rootCommandIndex + relativeCommandIndex);
        
        if (cmd->primitiveOffsetOrNegativeChildrenCount < 0)
        {
            locals.nbChildrenLeft = -cmd->primitiveOffsetOrNegativeChildrenCount;
        }
        else
        {
            locals.nbChildrenLeft = -100;
        }
        
        //locals.relativeMinDrawCommandIndex = -1; // avoid one init masked by initial max value of distance
        locals.relativeDrawCommandIndex = relativeCommandIndex;
        locals.distance = 1e7f;
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
    
    TDrawCommandIndex closestPositiveRelativeDrawCommandIndex() const
    {
        int8_t depth = _stackIndex;
        while (depth >= 0)
        {
            const THREAD Locals& locals = _stack[depth--];
            
            const auto cmdIndex = _rootCommandIndex + locals.relativeDrawCommandIndex;
            CONSTANT auto* drawCommand = _serialized.drawCommand(cmdIndex);
            if (drawCommand->primitiveOffsetOrNegativeChildrenCount < 0)
            {
                // a group
                return locals.relativeDrawCommandIndex;
            }
            else
            {
                // a prim
                CONSTANT auto* prim =  _serialized.primitive(drawCommand->primitiveOffsetOrNegativeChildrenCount);
                if (prim->sdfOperation() == SDFOperation::addition)
                {
                    return locals.relativeDrawCommandIndex;
                }
            }
            
        }
        
        return kInvalidCommandIndex;
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
        _minDistance = 1e5f;
        _minCmdIndex = -1;
    }
    
    float minDistance() const
    {
        return _minDistance;
    }
    
    bool hit() const
    {
        return _minCmdIndex != kInvalidCommandIndex;
    }
    
    TDrawCommandIndex minCmdIndex() const
    {
        return _minCmdIndex;
    }
    
    MaterialID minMaterialID() const
    {
        return _minMaterialID;
    }
    
    ObjectID minObjectID() const
    {
        return _minObjectID;
    }
    
    void setMinData(TDrawCommandIndex index, MaterialID materialID, ObjectID minObjectID)
    {
        _minCmdIndex = index;
        _minMaterialID = materialID;
        _minObjectID = minObjectID;
    }
    
    CullingInfo cullingInfo()
    {
        return _cullingInfo;
    }
    
    bool submitMinDistance(float dist)
    {
        if (dist < _minDistance)
        {
            _minDistance = dist;
            return _minDistance <= kDistanceEpsilon;
        }
        
        return false;
    }
    
private:
    CullingInfo _cullingInfo;
    
    float _minDistance = 1e5f;
    TDrawCommandIndex _minCmdIndex = -1;
    MaterialID _minMaterialID = kNoMaterialID;
    ObjectID _minObjectID = kInvalidObjectID;
};

INLINE float opSmoothUnion(float d1, float d2, float k)
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

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
            if (!locals.isCulled())
            {
                CONSTANT auto* cmd = stack.currentDrawCommand();
                auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                const bool isAdditive = (prim->sdfOperation() == SDFOperation::addition);
                float d = evaluatePrimitive<DistanceEvaluator, float>(distanceEvaluator, prim);
                
                if (auto parentLocals = stack.parentLocals())
                {
                    if (isAdditive)
                    {
                        // dAcculumated = min (d, dAcculumated)
                        if (prim->blendingFactor != 0.f)
                        {
                            d = opSmoothUnion(parentLocals->distance, d, prim->blendingFactor);
                        }
                        
                        if (d < parentLocals->distance)
                        {
                            parentLocals->distance = d;
                            parentLocals->relativeMinDrawCommandIndex = locals.relativeDrawCommandIndex;
                            parentLocals->flags = 0;
                        }
                    }
                    else
                    {
                        // dAcculumated = max(-d, dAcculumated)
                        d = -d;
                        if (d > parentLocals->distance)
                        {
                            parentLocals->distance = d;
                            parentLocals->relativeMinDrawCommandIndex = locals.relativeDrawCommandIndex;
                            parentLocals->flags = Locals::FFlags::fIsMinDrawCommandSubstractive;
                        }
                    }
                }
                else if (isAdditive)
                {
                    locals.distance = d;
                    visitor.submitMinDistance(d);
                }
            }

            stack.back();
        }
        else if (n > 0)
        {
            // groups could be culled too in future
            ASSERT(!locals.isCulled());
            
            // A inner Draw Command
            stack.push();
        }
        else
        {
            // == 0
            // -> a group and we just finished to go through the children
            const float dist = locals.distance;
            
            if (!locals.isMinDrawCommandSubstractive())
            {
                // positive
                if (visitor.submitMinDistance(dist))
                {
                    // hit of positive part
                    const auto relativeMinCmdIndex = locals.relativeMinDrawCommandIndex;
                    const auto cmdIndex = rootCommandIndex + relativeMinCmdIndex;
                    CONSTANT auto* cmd = serialized.drawCommand(cmdIndex);
                    ASSERT(cmd->primitiveOffsetOrNegativeChildrenCount >= 0);
                    
                    CONSTANT auto* prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                    
                    visitor.setMinData(cmdIndex, prim->materialId, prim->objectId);
                    break;
                }
            }
            else
            {
                if (visitor.submitMinDistance(dist))
                {
                    // hit of negative part

                    auto relativeMinCmdIndexForShadingPurpose = locals.relativeMinDrawCommandIndex;
                    const auto cmdIndexForShadingPurpose = rootCommandIndex + relativeMinCmdIndexForShadingPurpose;
                    CONSTANT auto* cmd = serialized.drawCommand(cmdIndexForShadingPurpose);
                    ASSERT(cmd->primitiveOffsetOrNegativeChildrenCount >= 0);
                    CONSTANT auto* prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                    
                    const auto relativeMinCmdIndex = stack.closestPositiveRelativeDrawCommandIndex();
                    const auto cmdIndex = rootCommandIndex + relativeMinCmdIndex;
                    
                    visitor.setMinData(cmdIndex, prim->materialId, prim->objectId);
                    break;
                }
            }
            
            if (auto parentLocals = stack.parentLocals())
            {
                parentLocals->distance = min(parentLocals->distance, dist);
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

INLINE void visitDrawCommandTree(float3 pt,
                    CONSTANT SerializedWorldObject& serialized,
                    TDrawCommandIndex rootCmdIndex,
                    THREAD Visitor& visitor)
{
    return _visitDrawCommandTree(pt, serialized, rootCmdIndex, visitor);
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
            if (visitor.submitMinDistance(d))
            {
                visitor.setMinData(serialized.drawCommandIndex(cmd), prim->materialId, prim->objectId);
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
    
    const auto materialID = visitor.minMaterialID();
    const auto objectID = visitor.minObjectID();
    
    // This should be a leaf primitive
    auto cmd = serialized.drawCommand(minCmdIndex);
    
    const TDrawCommandIndex startCmdIndex = minCmdIndex + cmd->ownerOffset;
    
    const auto subCullingInfo = cullingInfo.subCulling(tile.rootCommandIndex, startCmdIndex);
    ShadedPrimitive primitive { serialized, startCmdIndex, materialID, subCullingInfo };
    const auto color = shader.computeShade(primitive, ray, visitor.minDistance(), pt);
    
    return RayMarchResult { ray, objectID, color, d };
}

static CONSTANT constexpr size_t kNbSteps = 100;

struct RayMarchEvaluatorResult
{
    float d = 0.f;
    float3 pt;
    
    RayMarchEvaluatorResult() = default;
    RayMarchEvaluatorResult(Ray ray)
    : pt(ray.origin)
    {}
};

class RayMarchEvaluator
{
public:
    
    RayMarchEvaluator(CONSTANT SerializedWorldObject& serialized,
                      Ray ray,
                      TDrawCommandIndex cmdIndex,
                      THREAD Visitor& visitor)
    : _serialized(serialized), _ray(ray), _cmdIndex(cmdIndex), _visitor(visitor)
    {}
    
    template <typename TPrimitive>
    RayMarchEvaluatorResult evaluate(TPrimitive primitive) const
    {
        RayMarchEvaluatorResult res { _ray };

        for (size_t i=0; i < kNbSteps; ++i)
        {
            res.pt = _ray.pt(res.d);

            const float d = primitive.computeDistance(res.pt);
            if (_visitor.submitMinDistance(d))
            {
                auto cmd = _serialized.drawCommand(_cmdIndex);
                ASSERT(cmd->primitiveOffsetOrNegativeChildrenCount >= 0);
                auto p = _serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
                
                _visitor.setMinData(_cmdIndex, p->materialId, p->objectId);
                break;
            }
            
            res.d += _visitor.minDistance();
            
            if (res.d > _ray.maxLength)
            {
                break;
            }
        }
        
        return res;
    }
    
private:
    CONSTANT SerializedWorldObject& _serialized;
    const Ray _ray;
    const TDrawCommandIndex _cmdIndex;
    THREAD Visitor& _visitor;
};


INLINE RayMarchEvaluatorResult rayMarchOnePrimitive(CONSTANT SerializedWorldObject& serialized,
                          Ray ray,
                          TDrawCommandIndex cmdIndex,
                          THREAD Visitor& visitor)
{
    auto cmd = serialized.drawCommand(cmdIndex);
    ASSERT(cmd->primitiveOffsetOrNegativeChildrenCount >= 0);
    auto prim = serialized.primitive(cmd->primitiveOffsetOrNegativeChildrenCount);
    
    RayMarchEvaluator evaluator { serialized, ray, cmdIndex, visitor };
    return evaluatePrimitive<RayMarchEvaluator, RayMarchEvaluatorResult>(evaluator, prim);
}

#define ACTIVATE_ONE_PRIMITIVE_RAYMARCHING 0

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
        
#if ACTIVATE_ONE_PRIMITIVE_RAYMARCHING
        uint8_t lastRelativeCmdIndexNotCulled[2];
#endif
        
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
#if ACTIVATE_ONE_PRIMITIVE_RAYMARCHING
                    lastRelativeCmdIndexNotCulled[prim->operation] = i;
#endif
                }
            }
            
            cmd++;
        }
        
        if (nbObjectsPerOperation[size_t(SDFOperation::addition)] == 0)
        {
            return RayMarchResult { ray };
        }
        
        Visitor visitor;
        float d = 0.f;
        float3 pt = ray.origin;
        
        if ((tile.flags & Tile::Flags::fHasBlendedPrimitives) || (nbObjectsPerOperation[size_t(SDFOperation::substraction)] != 0))
        {
            for (size_t i=0; i < kNbSteps; ++i)
            {
                pt = ray.pt(d);
                
                visitor.reset(cullingInfo);
                
                visitDrawCommandTree(pt, _serialized, tile.rootCommandIndex, visitor);
                
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
        }
#if ACTIVATE_ONE_PRIMITIVE_RAYMARCHING
        else if (nbObjectsPerOperation[size_t(SDFOperation::addition)] == 1)
        {
            // look for the only cmd not culled
            const TDrawCommandIndex cmdIndex = tile.rootCommandIndex + lastRelativeCmdIndexNotCulled[0];
            
            // only one positive primitive to render
            const auto res = rayMarchOnePrimitive(_serialized,
                                                  ray,
                                                  cmdIndex,
                                                  visitor);
            d = res.d;
            pt = res.pt;
        }
#endif
        else
        {
            for (size_t i=0; i < kNbSteps; ++i)
            {
                pt = ray.pt(d);
                
                visitor.reset(cullingInfo);
                
                // only positive objects, can ignore the tree structure and iterate flat
                visitFlatCommandList(pt, _serialized, tile.rootCommandIndex, tile.nbCommands, visitor);

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
