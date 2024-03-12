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
class CullingNode;

class CullingNodeArrayPool;
class CullingNodeArray final
{
public:
    CullingNodeArray() = default;
    CullingNodeArray(size_t startIndex, size_t capacity);
    CullingNodeArray(CullingNodeArray&&);
    
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }
    
    const CullingNode* at(CullingNodeArrayPool& pool, size_t index) const;
    CullingNode* at(CullingNodeArrayPool& pool, size_t index);
    
    void push_back(CullingNodeArrayPool& pool, CullingNode*);
    
    CullingNodeArray& operator=(CullingNodeArray&&);
    
private:
    
    CullingNodeArray(const CullingNodeArray&) = delete;
    
    size_t _capacity = 0;
    size_t _size = 0;
    size_t _startIndex = 0;
};

struct CullingNode final
{
public:
    const Object3D* const object;
    const SDFOperation operation;
    const bool hasGeometry;
    const bool isCompound;
    
    const RectF box;
    RectF boxOfHierarchy; // including box
    
    CullingNodeArray positiveChildren;
    CullingNodeArray negativeChildren;
    
    CullingNode(const Object3D& object, const RectF& box);
};

class CullingNodeArrayPool final
{
public:
    void reserve(size_t size);
    
    CullingNodeArray allocate(size_t);
    
private:
    friend class CullingNodeArray;
    
    std::vector<CullingNode*> _scratch;
    size_t _availableIndex = 0;
};

INLINE const CullingNode* CullingNodeArray::at(CullingNodeArrayPool& pool, size_t index) const
{
    ASSERT(index < _size);
    return pool._scratch[_startIndex + index];
}

INLINE CullingNode* CullingNodeArray::at(CullingNodeArrayPool& pool, size_t index)
{
    ASSERT(index < _size);
    return pool._scratch[_startIndex + index];
}

INLINE void CullingNodeArray::push_back(CullingNodeArrayPool& pool, CullingNode* node)
{
    ASSERT(_size < _capacity);
    pool._scratch[_startIndex + _size++] = node;
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
    
    CullingNode* _addCullingTree(const Object3D& root);
    
    bool _encodeHierarchy(TileDescriptor& tileDescr, const CullingNode* node, const DrawCommand* owner);
    
    std::vector<CullingNode> _cullingTree;
    CullingNodeArrayPool _cullingNodeArrayPool;
    
    std::unordered_map<const Object3D*, TPrimitiveOffset> _objectToOffset;
    
    EncodingContextDelegate* _delegate = nullptr;
};
