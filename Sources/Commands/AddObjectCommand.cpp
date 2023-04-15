//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "AddObjectCommand.h"

AddObjectCommand::AddObjectCommand(World* world, const Object3DFactory::Ptr& factory)
: _world(world), _factory(factory)
{
}

void
AddObjectCommand::run()
{
    _object = _factory->make();
    
    if (_material == nullptr)
    {
        _material = _world->addMaterial(float4 { 1, 0, 0, 1 });
    }
    
    _object->setMaterial(_material);
    
    _world->addObject(_object);
}

void
AddObjectCommand::undo()
{
    _world->removeObject(_object);
}
