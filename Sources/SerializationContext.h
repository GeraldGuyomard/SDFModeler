//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "SerializedWorldObject.h"
#include <functional>

class ProjectedBB final
{
public:
    using Points = std::array<float3, 8>;
    Points projectedPoints;
    
    using RawPoints = float3[8];
    ProjectedBB(const RawPoints& pts);
};

class Object3D;
class World;
class Rect;

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
    void serializeObjectHeader(Tile& tile, const SerializationHeaderCallback&);
    
    const ProjectedBB* projectedBB(ObjectID) const;
    
    const float2& viewportSize() const { return _viewportSize; }
    
    Tile& tileAt(size_t x, size_t y) const;
    
private:
    const float4x4 _viewProjectionMatrix;
    const float2 _viewportSize;
    SerializedWorldObject& _serializedWorldObject;
    
    ObjectHeader* _availableObjectHeader;
    
    bool _addBBoxRecursive(const std::shared_ptr<Object3D>& root, const Rect& viewportRect);
    bool _addBBox(ObjectID id, const float4x4& worldViewProjMatrix, const BoundingBox& localBBox);
    
    std::unordered_map<ObjectID, ProjectedBB> _objectIDToProjectedBB;
};
