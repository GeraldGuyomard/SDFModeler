//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "GroupSelectionCommand.h"

GroupSelectionCommand::Entry::Entry(const Object3D::Ptr& object)
:object(object), parent(object->parent()), id(object->id())
{}

GroupSelectionCommand::GroupSelectionCommand(const Object3DSelection& selection)
{
    for (const auto& object : selection.objects())
    {
        _entries.emplace_back(object);
    }
}

void
GroupSelectionCommand::run()
{
    auto world = _entries.front().object->world();
    _group = std::make_shared<Object3D>(world);
    _group->setIsCompound(true);
    
    BoundingBox box;
    for (const auto& entry : _entries)
    {
        const auto b = entry.object->worldBoundingBoxOfHierarchy();
        box.add(b);
    }
    
    const auto pos = box.center();
    
    float4x4 t = float4x4_identity();
    setTranslation(t, pos);
    
    _group->setWorldTransform(t);
    
    for (const auto& entry : _entries)
    {
        const auto t = entry.object->worldTransform();
        _group->addChild(entry.object);
        entry.object->setWorldTransform(t);
    }
    
    _group->world()->rootObject()->addChild(_group);
}

void
GroupSelectionCommand::undo()
{
    for (const auto& entry : _entries)
    {
        const auto t = entry.object->worldTransform();
        entry.parent->addChild(entry.object);
        entry.object->setWorldTransform(t);
        entry.object->setId(entry.id);
    }
    
    _group->removeFromParent();
    _group = nullptr;
}
