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

class EncodingContext final
{
public:
    EncodingContext(
                         const std::shared_ptr<const World>& world,
                         const float4x4& viewProjectionMatrix,
                         const float2& viewportSize,
                         SerializedWorldObject& serializedWorldObject);
    
    const float4x4& viewProjectionMatrix() const { return _viewProjectionMatrix; }
    
    using EncodingPrimitiveCallback = std::function<size_t (EncodedPrimitive*)>;
    void encodePrimitive(const Object3D* object, const EncodingPrimitiveCallback&);
    TPrimitiveOffset encodedPrimitiveOffset(const Object3D* object) const;
    
    const ProjectedBB* projectedBB(const Object3D&) const;
    
    const float2& viewportSize() const { return _viewportRect.bottom; }
    SerializedWorldObject& serializedWorldObject() { return _serializedWorldObject; }
    
    bool isCulled(const Object3D& object, const RectF& tileRect) const;
    
    void encodePrimitives(const Object3D& root, uint32_t depth);
    
    void writePrimitiveDrawCommand(TPrimitiveOffset primitiveOffset);
    DrawCommand& writeGroupDrawCommand();
    void cancelLastDrawCommand();
    
    size_t availableCommandIndex() const { return _availableDrawCommandIndex; }
    
private:
    const float4x4 _viewProjectionMatrix;
    const RectF _viewportRect;
    SerializedWorldObject& _serializedWorldObject;
    
    TPrimitiveOffset _availableHeaderOffset = 0;
    size_t _nbPrimitivesSerialized = 0;
    size_t _availableDrawCommandIndex = 0;
    
    bool _addBBoxRecursive(const std::shared_ptr<Object3D>& root, const RectF& viewportRect);
    bool _addBBox(const Object3D* object, const float4x4& worldViewProjMatrix, const BoundingBox& localBBox);
    
    std::unordered_map<const Object3D*, ProjectedBB> _objectToProjectedBB;
    std::unordered_map<const Object3D*, TPrimitiveOffset> _objectToOffset;
};
