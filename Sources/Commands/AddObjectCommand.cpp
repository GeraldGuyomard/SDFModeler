//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "AddObjectCommand.h"

AddObjectCommand::AddObjectCommand(
                                   const Object3D::Ptr& parent,
                                   const Object3DFactory::Ptr& factory)
: _parent(parent), _factory(factory)
{
}

void
AddObjectCommand::run()
{
    auto world = _parent->world();
    
    _object = _factory->make(world);
    
    if (_material == nullptr)
    {
        _material = world->addMaterial(float4 { 1, 0, 0, 1 });
    }
    
    _object->setMaterial(_material);
    
    _parent->addChild(_object);
}

void
AddObjectCommand::undo()
{
    _object->removeFromParent();
}
