//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "TransformObjectCommand.h"

TransformObjectCommand::TransformObjectCommand(const Object3D::Ptr& object, const float4x4& transform)
: _object(object), _transform(transform)
{}

void
TransformObjectCommand::run()
{
    const auto transform = _object->worldTransform();
    _object->setWorldTransform(_transform);
    _transform = transform;
}

void
TransformObjectCommand::undo()
{
    run();
}
