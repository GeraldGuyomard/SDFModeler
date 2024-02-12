//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "TransformObjectCommand.h"

TransformObjectCommand::Entry::Entry(const Object3D::Ptr& object)
: object(object), transform(object->worldTransform())
{
}


TransformObjectCommand::TransformObjectCommand(const std::vector<Entry>& state)
: _state(state)
{}


void
TransformObjectCommand::undoRedo()
{
    auto saved = _state;
    for (auto& entry: saved)
    {
        entry.transform = entry.object->worldTransform();
    }
    
    for (const auto& entry: _state)
    {
        entry.object->setWorldTransform(entry.transform);
    }
    
    _state = saved;
}
