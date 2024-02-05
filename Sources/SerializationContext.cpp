//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#include "SerializationContext.h"
#include "Object3D.h"

ProjectedBB::ProjectedBB(const RawPoints& pts)
{
    for (size_t i=0; i < 8; ++i)
    {
        projectedPoints[i] = pts[i];
    }
}

bool
SerializationContext::_addBBoxRecursive(const std::shared_ptr<Object3D>& root)
{
    const auto worldViewProjMatrix = _viewProjectionMatrix * root->worldTransform();
    const auto box = root->localBoundingBox();
    
    const bool thisVisible = _addBBox(root->directID(), worldViewProjMatrix, box);
    
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
        if (_addBBoxRecursive(child))
        {
            positiveChildVisible = true;
        }
    }
    
    if (positiveChildVisible)
    {
        for (const auto& child : negativeChildren)
        {
            if (!child->isCulled(_viewProjectionMatrix))
            {
                _addBBoxRecursive(child);
            }
        }
    }
    
    return thisVisible;
}


bool
SerializationContext::_addBBox(ObjectID id, const float4x4& worldViewProjMatrix, const BoundingBox& localBBox)
{
    float3 pts[8];
    localBBox.points(pts);
    
    bool visible = false;
    for (int i=0; i < 8; ++i)
    {
        float3& pt = pts[i];
        
        // NDC in metal is x, y between [0, 1]
        // and z between [0, 1]
        const float4 p = worldViewProjMatrix * float4 { pt.x, pt.y, pt.z, 1.f };
        pt = p.xyz / p.w;
        
        const bool inFrustrum = (pt.x >= -1.f) && (pt.x <= 1.f) && (pt.y >= -1.f) && (pt.y <= 1.f) && (pt.z >= 0.f) && (pt.z <= 1.f);
        
        if (inFrustrum)
        {
            visible = true;
        }
    }
    
    if (!visible)
    {
        return false;
    }
    
    _objectIDToProjectedBB.emplace(std::pair {id, ProjectedBB{pts}});
    
    return true;
}

const ProjectedBB*
SerializationContext::projectedBB(ObjectID id) const
{
    const auto it = _objectIDToProjectedBB.find(id);
    return (it != _objectIDToProjectedBB.end()) ? &it->second : nullptr;
}


SerializationContext::SerializationContext(const std::shared_ptr<const World>& world,
                                           const float4x4& viewProjectionMatrix,
                                           const float2& viewportSize,
                                           SerializedWorldObject& serializedWorldObject)
:
_viewProjectionMatrix(viewProjectionMatrix),
_viewportSize(viewportSize),
_serializedWorldObject(serializedWorldObject)
{
    _serializedWorldObject.tileSize = _viewportSize;
    _serializedWorldObject.numTileColumns = 1;
    _serializedWorldObject.numTileRows = 1;
    
    const size_t nbTiles = _serializedWorldObject.numTileColumns * _serializedWorldObject.numTileRows;
    for (size_t i=0 ; i < nbTiles; ++i)
    {
        Tile& tile = _serializedWorldObject.tiles[i];
        tile.objectCount = 0;
        tile.offsetInBuffer = 0;
        
    }
    _availableObjectHeader = reinterpret_cast<ObjectHeader*>(&_serializedWorldObject.buffer[0]);
    
    _addBBoxRecursive(world->rootObject());
}

Tile&
SerializationContext::tileAt(size_t x, size_t y) const
{
    const size_t index = (y * _serializedWorldObject.numTileColumns) + x;
    return _serializedWorldObject.tiles[index];
}

void
SerializationContext::serializeObjectHeader(Tile& tile, const SerializationHeaderCallback& cb)
{
    const size_t size = cb(_availableObjectHeader);
    
    _availableObjectHeader->byteSize = uint32_t(size);
    _availableObjectHeader = ObjectHeader::next(_availableObjectHeader);
    
    ++tile.objectCount;
}
