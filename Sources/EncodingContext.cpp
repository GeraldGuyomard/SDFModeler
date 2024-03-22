//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#include "EncodingContext.h"
#include "Object3D.h"
#include "RectF.h"

CullingNodeArray::CullingNodeArray(size_t startIndex, size_t capacity)
: _startIndex(startIndex), _capacity(capacity)
{}

CullingNodeArray::CullingNodeArray(CullingNodeArray&& other)
: _startIndex(other._startIndex), _capacity(other._capacity)
{
    other._startIndex = 0;
    other._capacity = 0;
}

CullingNodeArray&
CullingNodeArray::operator=(CullingNodeArray&& other)
{
    _startIndex = other._startIndex;
    _capacity = other._capacity;
    _size = other._size;
    
    other._startIndex = 0;
    other._capacity = 0;
    other._size = 0;
    
    return *this;
}

void
CullingNodeArrayPool::reserve(size_t size)
{
    _scratch.resize(size);
}

CullingNodeArray
CullingNodeArrayPool::allocate(size_t size)
{
    const size_t requiredScratchSize = _availableIndex + size;
    if (_scratch.size() < requiredScratchSize)
    {
        _scratch.resize(requiredScratchSize);
    }
    
    CullingNodeArray array { _availableIndex, size };
    _availableIndex += size;
    
    return array;
}

CullingNode::CullingNode(const Object3D& object, const RectF& box)
: object(&object),
operation(object.operation()),
hasGeometry(object.geometryType() != nullptr),
isCompound(object.isCompound()),
box(box),
boxOfHierarchy(box)
{}

CullingNode* EncodingContext::_addCullingTree(const Object3D& root)
{
    if (!shouldEncode(root))
    {
        return nullptr;
    }
    
    const auto localBBox = root.localBoundingBox();
    RectF box;
    
    if (!localBBox.empty())
    {
        const auto worldViewProjMatrix = _viewProjectionMatrix * root.worldTransform();
        float3 pts[8];
        localBBox.points(pts);
        
        for (size_t i=0; i < 8; ++i)
        {
            float3& pt = pts[i];
            
            // NDC in metal is x, y between [0, 1]
            // and z between [0, 1]
            const float4 p = worldViewProjMatrix * float4 { pt.x, pt.y, pt.z, 1.f };
            pt = p.xyz / p.w;
            
            const float x = (1.f + pt.x) * 0.5f * _viewportRect.bottomRight.x;
            const float y = (1.f - pt.y) * 0.5f * _viewportRect.bottomRight.y;
            
            const bool inFrustrum = (pt.z >= 0.f) && (pt.z <= 1.f);
            if (inFrustrum)
            {
                box.add(float2 {x, y});
            }
        }
    }
    
    _cullingTree.emplace_back( root, box );
    auto& node = _cullingTree.back();
    
    const auto& children = root.children();
    const size_t childCount = children.size();
    
    // Positive first
    node.positiveChildren = _cullingNodeArrayPool.allocate(childCount);
    node.negativeChildren = _cullingNodeArrayPool.allocate(childCount);
    
    for (const auto& child: children)
    {
        auto* childNode = _addCullingTree(*child);
        if (childNode != nullptr)
        {
            const auto op = operation(*child);
            if (op == SDFOperation::addition)
            {
                node.boxOfHierarchy = node.boxOfHierarchy.makeUnion(childNode->boxOfHierarchy);
                node.positiveChildren.push_back(_cullingNodeArrayPool, childNode);
            }
            else
            {
                assert(op == SDFOperation::substraction);
                node.negativeChildren.push_back(_cullingNodeArrayPool, childNode);
            }
        }
    }
    
    return &node;
}

namespace
{
    size_t nodeCount(const Object3D& object)
    {
        size_t n = 1;
        
        for(const auto& child : object.children())
        {
            n += nodeCount(*child);
        }
        
        return n;
    }
}

void
EncodingContext::buildCullingTree(const Object3D& root)
{
    _cullingTree.clear();
    
    const size_t n = nodeCount(root);
    _cullingTree.reserve(n);
    
    _addCullingTree(root);
}

EncodingContext::EncodingContext(const std::shared_ptr<const World>& world,
                                           const float4x4& viewProjectionMatrix,
                                           const float2& viewportSize,
                                            const float2& tileSize,
                                           SerializedWorldObject& serializedWorldObject)
:
_viewProjectionMatrix(viewProjectionMatrix),
_viewportRect(float2 { 0, 0 }, viewportSize),
_serializedWorldObject(serializedWorldObject)
{
    auto env = world->environment();
    _serializedWorldObject.grid = env->typedGeometry();
    
    auto envTransform = world->rootObject()->worldTransform() * env->worldTransform();
    auto gridPos = translation(envTransform);
    gridPos.y -= 0.35f;
    setTranslation(envTransform, gridPos);
    
    _serializedWorldObject.grid.setTransform(envTransform);
    
    _serializedWorldObject.tileSize = tileSize;
    
    const auto& vpSize = this->viewportSize();
    
    _serializedWorldObject.numTileColumns = ceilf(vpSize.x / _serializedWorldObject.tileSize.x);
    _serializedWorldObject.numTileRows = ceilf(vpSize.y / _serializedWorldObject.tileSize.y);
    
    const size_t nbTiles = _serializedWorldObject.numTileColumns * _serializedWorldObject.numTileRows;
    if (nbTiles > kMaxTiles)
    {
        // reducing
        if (_serializedWorldObject.numTileColumns >= _serializedWorldObject.numTileRows)
        {
            // horizontal
            _serializedWorldObject.numTileRows = floorf(kMaxTiles / _serializedWorldObject.numTileColumns);
        }
        else
        {
            // vertical
            _serializedWorldObject.numTileColumns = floorf(kMaxTiles / _serializedWorldObject.numTileRows);
        }
        
        assert((_serializedWorldObject.numTileColumns * _serializedWorldObject.numTileRows) < kMaxTiles);
    }
    
    size_t i=0;
    float2 minPt = { 0, 0 };
    
    for (size_t y=0 ; y < _serializedWorldObject.numTileRows; ++y)
    {
        minPt.x = 0;
        
        for (size_t x=0; x < _serializedWorldObject.numTileColumns; ++x)
        {
            Tile& tile = _serializedWorldObject.tiles[i++];
            tile.rootCommandIndex = -1;
            tile.minPt = minPt;
            tile.maxPt = minPt + _serializedWorldObject.tileSize;
            tile.maxPt = min(tile.maxPt, vpSize);
            
            minPt.x += _serializedWorldObject.tileSize.x;
        }
        
        minPt.y += _serializedWorldObject.tileSize.y;
    }
}

void
EncodingContext::encodePrimitive(const Object3D* object, const EncodingPrimitiveCallback& cb)
{
    assert (_objectToOffset.find(object) == _objectToOffset.end());
    
    _nbPrimitivesSerialized++;
    
    const auto offset = _availableHeaderOffset;
    auto* header = reinterpret_cast<EncodedPrimitive*>(_serializedWorldObject.primitivesBuffer + offset);
    const size_t size = cb(header);
    
    _objectToOffset.emplace(std::pair(object, offset));
    
    _availableHeaderOffset += size;
    assert(size_t(_availableHeaderOffset) <= kPrimitivesBufferSize);
}

TPrimitiveOffset
EncodingContext::encodedPrimitiveOffset(const Object3D* object) const
{
    const auto it = _objectToOffset.find(object);
    if (it != _objectToOffset.end())
    {
        const TPrimitiveOffset offset = it->second;
        return offset;
    }
    else
    {
        return kInvalidPrimitiveOffset;
    }
}

void
EncodingContext::encodePrimitives(const Object3D& root)
{
    root.selfEncode(*this);
    
    for (const auto& child : root.children())
    {
        encodePrimitives(*child);
    }
}

void
EncodingContext::encodeHierarchy(TileDescriptor& tileDescr, const DrawCommand* owner)
{
    if (!_cullingTree.empty())
    {
        _encodeHierarchy(tileDescr, &_cullingTree[0], owner);
    }
}

bool
EncodingContext::_encodeHierarchy(TileDescriptor& tileDescr, const CullingNode* node, const DrawCommand* owner)
{
    // culling of hierarchy
    if (!tileDescr.tileRect.intersects(node->boxOfHierarchy))
    {
        return false;
    }
    
    if (node->hasGeometry)
    {
        assert(node->positiveChildren.empty());
        assert(node->negativeChildren.empty());
        
        // culling
        if (!tileDescr.tileRect.intersects(node->box))
        {
            return false;
        }
        
        if (node->object->blendingFactor() != 0.f)
        {
            tileDescr.tile.flags |= Tile::Flags::fHasBlendedPrimitives;
        }
        
        const auto myPrimitiveOffset = encodedPrimitiveOffset(node->object);
        writePrimitiveDrawCommand(myPrimitiveOffset, owner);
        return true;
    }
    else
    {
        // a compound or a group
        if (!node->positiveChildren.empty())
        {
            auto& cmd = writeGroupDrawCommand(owner);
            
            if (node->isCompound)
            {
                owner = &cmd;
            }
            
            int16_t n = 0;
            const size_t nbPositiveChildren = node->positiveChildren.size();
            for (size_t i=0; i < nbPositiveChildren; ++i)
            {
                if (_encodeHierarchy(tileDescr, node->positiveChildren.at(_cullingNodeArrayPool, i), owner))
                {
                    ++n;
                }
            }
            
            if (n != 0)
            {
                const size_t nbNegativeChildren = node->negativeChildren.size();
                for (size_t i=0; i < nbNegativeChildren; ++i)
                {
                    if (_encodeHierarchy(tileDescr, node->negativeChildren.at(_cullingNodeArrayPool, i), owner))
                    {
                        ++n;
                    }
                }
                
                cmd.primitiveOffsetOrNegativeChildrenCount = -n;
                return true;
            }
            else
            {
                cancelLastDrawCommand();
            }
        }
    }
    
    return false;
}

void
EncodingContext::writePrimitiveDrawCommand(TPrimitiveOffset primitiveOffset, const DrawCommand* owner)
{
    assert(primitiveOffset >= 0);
    
    assert(_availableDrawCommandIndex < kDrawCommandArraySize);
    
    auto& cmd = _serializedWorldObject.drawCommands[_availableDrawCommandIndex++];
    cmd.primitiveOffsetOrNegativeChildrenCount = primitiveOffset;
    cmd.ownerOffset = (owner != nullptr) ? (owner - &cmd) : 0;
}

DrawCommand&
EncodingContext::writeGroupDrawCommand(const DrawCommand* owner)
{
    assert(_availableDrawCommandIndex < kDrawCommandArraySize);
    
    auto& cmd = _serializedWorldObject.drawCommands[_availableDrawCommandIndex++];
    cmd.ownerOffset = (owner != nullptr) ? (owner - &cmd) : 0;
    return cmd;
}

void
EncodingContext::cancelLastDrawCommand()
{
    assert(_availableDrawCommandIndex > 0);
    --_availableDrawCommandIndex;
}

bool
EncodingContextDelegate::shouldEncode(const Object3D&) const
{
    return true;
}

SDFOperation
EncodingContextDelegate::operation(const Object3D& object) const
{
    return object.operation();
}

void
EncodingContext::setDelegate(EncodingContextDelegate* delegate)
{
    _delegate = delegate;
}

bool
EncodingContext::shouldEncode(const Object3D& object) const
{
    return (_delegate == nullptr) || _delegate->shouldEncode(object);
}

SDFOperation
EncodingContext::operation(const Object3D& object) const
{
    if (_delegate != nullptr)
    {
        return _delegate->operation(object);
    }
    else
    {
        return object.operation();
    }
}

void
EncodingContext::encode(const World& world, Materials& materials)
{
    auto& serialized = serializedWorldObject();
    
    auto root = world.rootObject();
    encodePrimitives(*root);
    buildCullingTree(*root);
    
    const size_t nbTiles = serialized.numTileRows * serialized.numTileColumns;
    
    for (size_t index=0; index < nbTiles; ++index)
    {
        auto& tile = serialized.tiles[index];
        tile.rootCommandIndex = availableCommandIndex();
        
        TileDescriptor descr { tile };
        
        encodeHierarchy(descr, nullptr);
        
        tile.nbCommands = availableCommandIndex() - tile.rootCommandIndex;
        if (tile.nbCommands == 0)
        {
            tile.rootCommandIndex = -1;
        }
    }
    
    const auto& allMaterials = world.materials();
    
    materials.nbMaterials = allMaterials.size();
    
    for (const auto& mat : allMaterials)
    {
        materials.material[mat->id()] = mat->material();
    }
}
