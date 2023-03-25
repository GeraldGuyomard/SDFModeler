//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Object3D.h"

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
        object->serialize(serializedScene, p);
    }
}
