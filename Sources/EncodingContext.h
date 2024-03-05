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

class ChildReorderingArray;
class ChildReorderingArrayChunk final
{
public:
    ChildReorderingArrayChunk(ChildReorderingArray& array, size_t startIndex, size_t capacity);
    ChildReorderingArrayChunk(ChildReorderingArrayChunk&&);
    ~ChildReorderingArrayChunk();
    
    size_t size() const { return _size; }
    uint8_t operator[](size_t index) const;
    uint8_t& operator[](size_t index);
    
    void push_back(uint8_t v);
    
private:
    
    ChildReorderingArrayChunk(const ChildReorderingArrayChunk&) = delete;
    
    ChildReorderingArray* _array;
    const size_t _capacity;
    size_t _size = 0;
    const size_t _startIndex;
};

class ChildReorderingArray final
{
public:
    ChildReorderingArray(size_t reserve = 128);
    
    ChildReorderingArrayChunk allocate(size_t);
    
private:
    friend class ChildReorderingArrayChunk;
    
    std::vector<uint8_t> _scratch;
    size_t _availableIndex = 0;
};

INLINE uint8_t ChildReorderingArrayChunk::operator[](size_t index) const
{
    ASSERT(index < _size);
    return _array->_scratch[_startIndex + index];
}

INLINE uint8_t& ChildReorderingArrayChunk::operator[](size_t index)
{
    ASSERT(index < _size);
    return _array->_scratch[_startIndex + index];
}

INLINE void ChildReorderingArrayChunk::push_back(uint8_t v)
{
    ASSERT(_size < _capacity);

    _array->_scratch[_startIndex + _size++] = v;
}

class EncodingContext final
{
public:
    EncodingContext(
                         const std::shared_ptr<const World>& world,
                         const float4x4& viewProjectionMatrix,
                         const float2& viewportSize,
                         const float2& tileSize,
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
    
    void writePrimitiveDrawCommand(TPrimitiveOffset primitiveOffset, const DrawCommand* owner);
    DrawCommand& writeGroupDrawCommand(const DrawCommand* owner);
    void cancelLastDrawCommand();
    
    size_t availableCommandIndex() const { return _availableDrawCommandIndex; }
    
    ChildReorderingArray& childOrderingArray() { return _childOrderingArray; }
    
    using EncodingFilter = std::function<bool(const Object3D&)>;
    void setEncodingFilter(const EncodingFilter& filter);
    
    bool shouldEncode(const Object3D&) const;
    
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
    
    ChildReorderingArray _childOrderingArray;
    EncodingFilter _encodingFilter;
};
