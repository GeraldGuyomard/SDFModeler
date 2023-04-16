//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "RemoveObjectCommand.h"

RemoveObjectCommand::RemoveObjectCommand(const Object3D::Ptr& object)
: _object(object), _parent(object->parent())
{
}

void
RemoveObjectCommand::run()
{
    _object->removeFromParent();
}

void
RemoveObjectCommand::undo()
{
    _parent->addChild(_object);
}
