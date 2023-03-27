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
World::addObject(Object3D::Ptr object)
{
    _objects.push_back(std::move(object));
}

void
World::serialize(SerializedScene& serializedScene) const
{
    serializedScene.objectCount = 0;
    
    uint8_t* p = reinterpret_cast<uint8_t*>(&(serializedScene.buffer));
    
    for (const auto& object : _objects)
    {
        const size_t size = object->serialize(p);
        p += size;
        serializedScene.objectCount++;
    }
}
