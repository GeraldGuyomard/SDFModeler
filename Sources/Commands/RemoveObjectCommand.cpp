//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "RemoveObjectCommand.h"

RemoveObjectCommand::Entry::Entry(const Object3D::Ptr& object)
: object(object), parent(object->parent())
{}

RemoveObjectCommand::RemoveObjectCommand(const Object3DSelection& selection)
{
    for (const auto& object : selection.objects())
    {
        _entries.emplace_back(object);
    }
}

void
RemoveObjectCommand::run()
{
    for (const auto& entry: _entries)
    {
        entry.object->removeFromParent();
    }
}

void
RemoveObjectCommand::undo()
{
    for (const auto& entry: _entries)
    {
        entry.parent->addChild(entry.object);
    }
}
