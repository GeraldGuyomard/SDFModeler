//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "TransformObjectCommand.h"

TransformObjectCommand::TransformObjectCommand(const Object3D::Ptr& object)
: _object(object), _initialObjectTransform(object->worldTransform())
{
    _transform = _initialObjectTransform;
}

void
TransformObjectCommand::setTransform(const float4x4& transform)
{
    _transform = transform;
}

void
TransformObjectCommand::run()
{
    _object->setWorldTransform(_transform);
}

void
TransformObjectCommand::undo()
{
    _object->setWorldTransform(_initialObjectTransform);
}
