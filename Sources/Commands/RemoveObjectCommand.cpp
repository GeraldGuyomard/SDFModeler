//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "RemoveObjectCommand.h"

RemoveObjectCommand::RemoveObjectCommand(World* world, const Object3D::Ptr& object)
: _world(world), _object(object), _parent(object->parent())
{
}

void
RemoveObjectCommand::run()
{
    if (_parent != nullptr)
    {
        _object->removeFromParent();
    }
    else
    {
        _world->removeObject(_object);
    }
}

void
RemoveObjectCommand::undo()
{
    if (_parent != nullptr)
    {
        _parent->addChild(_object);
    }
    else
    {
        _world->addObject(_object);
    }
}
