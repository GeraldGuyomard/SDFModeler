//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Object3D.h"

Grid3D::Grid3D(const Grid& grid)
: _inherited(grid)
{}

void
Object3DCollection::addObject(Object3D::Ptr object)
{
    _objects.push_back(std::move(object));
}

void
Object3DCollection::serialize(SerializedObjects& serializedObjects) const
{
    serializedObjects.objectCount = 0;
    
    uint8_t* p = reinterpret_cast<uint8_t*>(&(serializedObjects.buffer));
    
    for (const auto& object : _objects)
    {
        const size_t size = object->serialize(p);
        p += size;
        serializedObjects.objectCount++;
    }
}

void
World::serialize(SerializedWorld& serializedWorld) const
{
    _content.serialize(serializedWorld.content);
    _environment.serialize(serializedWorld.environment);
}
