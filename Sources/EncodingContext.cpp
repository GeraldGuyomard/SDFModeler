//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#include "EncodingContext.h"
#include "Object3D.h"
#include "RectF.h"

ProjectedBB::ProjectedBB(const RawPoints& pts, const RectF& viewportRect)
{
    for (size_t i=0; i < 8; ++i)
    {
        const auto& pt = pts[i];
        projectedPoints[i] = pt;
        
        // x => -1, 1 -> y => 1, -1
        float2 p { pt.x, pt.y };
        p.x = (1.f + p.x) * 0.5f * viewportRect.bottom.x;
        p.y = (1.f - p.y) * 0.5f * viewportRect.bottom.y;
        
        boundingBoxInViewportSpace.add(p);
    }
    
    boundingBoxInViewportSpace = boundingBoxInViewportSpace.makeIntersection(viewportRect);
}

bool
EncodingContext::_addBBoxRecursive(const std::shared_ptr<Object3D>& root, const RectF& viewportRect)
{
    const auto worldViewProjMatrix = _viewProjectionMatrix * root->worldTransform();
    const auto box = root->localBoundingBox();
    
    const bool thisVisible = _addBBox(root.get(), worldViewProjMatrix, box);
    
    std::vector<Object3D::Ptr> positiveChildren;
    std::vector<Object3D::Ptr> negativeChildren;

    for (const auto& child : root->children())
    {
        switch (child->operation())
        {
            case SDFOperation::addition:
            {
                positiveChildren.push_back(child);
                break;
            }

            case SDFOperation::substraction:
            {
                negativeChildren.push_back(child);
                break;
            }
                
            default: break;
        }
    }
    
    bool positiveChildVisible = false;
    for (const auto& child : positiveChildren)
    {
        if (_addBBoxRecursive(child, viewportRect))
        {
            positiveChildVisible = true;
        }
    }
    
    if (positiveChildVisible)
    {
        for (const auto& child : negativeChildren)
        {
            _addBBoxRecursive(child, viewportRect);
        }
    }
    
    return thisVisible;
}


bool
EncodingContext::_addBBox(const Object3D* object, const float4x4& worldViewProjMatrix, const BoundingBox& localBBox)
{
    if (localBBox.empty())
    {
        return false;
    }
    
    float3 pts[8];
    localBBox.points(pts);
    
    bool inFrustrum = false;
    
    for (int i=0; i < 8; ++i)
    {
        float3& pt = pts[i];
        
        // NDC in metal is x, y between [0, 1]
        // and z between [0, 1]
        const float4 p = worldViewProjMatrix * float4 { pt.x, pt.y, pt.z, 1.f };
        pt = p.xyz / p.w;
        
        if (!inFrustrum)
        {
            inFrustrum = (pt.z >= 0.f) && (pt.z <= 1.f);
        }
    }
    
    if (!inFrustrum)
    {
        return false;
    }
    
    const ProjectedBB bb {pts, _viewportRect};
    if (bb.boundingBoxInViewportSpace.empty())
    {
        return false;
    }
    
    _objectToProjectedBB.emplace(std::pair {object, bb});
    
    return true;
}

const ProjectedBB*
EncodingContext::projectedBB(const Object3D& object) const
{
    const auto it = _objectToProjectedBB.find(&object);
    return (it != _objectToProjectedBB.end()) ? &it->second : nullptr;
}


EncodingContext::EncodingContext(const std::shared_ptr<const World>& world,
                                           const float4x4& viewProjectionMatrix,
                                           const float2& viewportSize,
                                           SerializedWorldObject& serializedWorldObject)
:
_viewProjectionMatrix(viewProjectionMatrix),
_viewportRect(float2 { 0, 0 }, viewportSize),
_serializedWorldObject(serializedWorldObject)
{
    //const float2 kDefaultTileSize { 128, 128 };
    const float2 kDefaultTileSize { 256, 256 };
    //const float2 kDefaultTileSize { 1024, 1024 };
    
    _serializedWorldObject.tileSize = kDefaultTileSize;
    
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
    
    const RectF viewportRect { float2 { 0, 0 }, vpSize };
    
    _addBBoxRecursive(world->rootObject(), viewportRect);
}

bool EncodingContext::isCulled(const Object3D& object, const RectF& tileRect) const
{
    const auto* box = projectedBB(object);
    if (box == nullptr)
    {
        return true;
    }
    
    return !box->boundingBoxInViewportSpace.intersects(tileRect);
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
EncodingContext::writePrimitiveDrawCommand(TPrimitiveOffset primitiveOffset)
{
    assert(primitiveOffset >= 0);
    
    assert(_availableDrawCommandIndex < kDrawCommandArraySize);
    
    DrawCommand& cmd = _serializedWorldObject.drawCommands[_availableDrawCommandIndex++];
    cmd.primitiveOffset = primitiveOffset;
}

DrawCommand&
EncodingContext::writeGroupDrawCommand()
{
    assert(_availableDrawCommandIndex < kDrawCommandArraySize);
    
    DrawCommand& cmd = _serializedWorldObject.drawCommands[_availableDrawCommandIndex++];
    
    return cmd;
}

void
EncodingContext::cancelLastDrawCommand()
{
    assert(_availableDrawCommandIndex > 0);
    --_availableDrawCommandIndex;
}

