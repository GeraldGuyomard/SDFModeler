//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "SerializedWorldObject.h"
#include "RectF.h"

#include <functional>

class ProjectedBB final
{
public:
    using Points = std::array<float3, 8>;
    Points projectedPoints;
    RectF boundingBoxInViewportSpace;
    
    using RawPoints = float3[8];
    ProjectedBB(const RawPoints& pts, const RectF& viewportRect);
};

class Object3D;
class World;
class RectF;

class SerializationContext final
{
public:
    SerializationContext(
                         const std::shared_ptr<const World>& world,
                         const float4x4& viewProjectionMatrix,
                         const float2& viewportSize,
                         SerializedWorldObject& serializedWorldObject);
    
    const float4x4& viewProjectionMatrix() const { return _viewProjectionMatrix; }
    
    using SerializationHeaderCallback = std::function<size_t (ObjectHeader*)>;
    TPrimitiveOffset serializeObjectHeader(Tile& tile, const Object3D* object, const SerializationHeaderCallback&);
    
    const ProjectedBB* projectedBB(const Object3D&) const;
    
    const float2& viewportSize() const { return _viewportRect.bottom; }
    SerializedWorldObject& serializedWorldObject() { return _serializedWorldObject; }
    
    bool isCulled(const Object3D& object, const RectF& tileRect) const;
    
    size_t currentIndex() const { return _currentIndex; }
    void writePrimitiveOffset(TPrimitiveOffset offset);
    
private:
    const float4x4 _viewProjectionMatrix;
    const RectF _viewportRect;
    SerializedWorldObject& _serializedWorldObject;
    
    TPrimitiveOffset _availableHeaderOffset = 0;
    size_t _nbPrimitivesSerialized = 0;
    size_t _currentIndex = 0;
    
    bool _addBBoxRecursive(const std::shared_ptr<Object3D>& root, const RectF& viewportRect);
    bool _addBBox(const Object3D* object, const float4x4& worldViewProjMatrix, const BoundingBox& localBBox);
    
    std::unordered_map<const Object3D*, ProjectedBB> _objectToProjectedBB;
    std::unordered_map<const Object3D*, TPrimitiveOffset> _objectToOffset;
};
