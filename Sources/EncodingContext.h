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

class EncodingContextDelegate
{
public:
    virtual ~EncodingContextDelegate() = default;
    virtual bool shouldEncode(const Object3D&) const;
    virtual SDFOperation operation(const Object3D&) const;
};

class TileDescriptor;

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
    
    const float2& viewportSize() const { return _viewportRect.bottomRight; }
    SerializedWorldObject& serializedWorldObject() { return _serializedWorldObject; }
    
    void encodePrimitives(const Object3D& root);
    void encodeHierarchy(TileDescriptor& tileDescr, const DrawCommand* owner);
    
    void writePrimitiveDrawCommand(TPrimitiveOffset primitiveOffset, const DrawCommand* owner);
    DrawCommand& writeGroupDrawCommand(const DrawCommand* owner);
    void cancelLastDrawCommand();
    
    size_t availableCommandIndex() const { return _availableDrawCommandIndex; }
    
    void buildCullingTree(const Object3D& root);
    
    void setDelegate(EncodingContextDelegate* delegate);
    
    bool shouldEncode(const Object3D&) const;
    SDFOperation operation(const Object3D&) const;
    
private:
    const float4x4 _viewProjectionMatrix;
    const RectF _viewportRect;
    
    SerializedWorldObject& _serializedWorldObject;
    
    TPrimitiveOffset _availableHeaderOffset = 0;
    size_t _nbPrimitivesSerialized = 0;
    size_t _availableDrawCommandIndex = 0;
    
    struct CullingNode final
    {
    public:
        const Object3D* const object;
        const SDFOperation operation;
        const bool hasGeometry;
        const bool isCompound;
        
        const RectF box;
        RectF boxOfHierarchy; // including box
        
        std::vector<CullingNode*> positiveChildren;
        std::vector<CullingNode*> negativeChildren;
        
        CullingNode(const Object3D& object, const RectF& box);
    };
    
    CullingNode* _addCullingTree(const Object3D& root);
    
    bool _encodeHierarchy(TileDescriptor& tileDescr, const CullingNode* node, const DrawCommand* owner);
    
    std::vector<CullingNode> _cullingTree;
    std::vector<CullingNode*> _childrenArray;
    
    std::unordered_map<const Object3D*, TPrimitiveOffset> _objectToOffset;
    
    EncodingContextDelegate* _delegate = nullptr;
};
