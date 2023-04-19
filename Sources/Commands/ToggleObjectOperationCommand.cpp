//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "ToggleObjectOperationCommand.h"

ToggleObjectOperationCommand::ToggleObjectOperationCommand(const Object3D::Ptr& object)
: _object(object)
{
}

void
ToggleObjectOperationCommand::run()
{
    const auto currentOp = _object->operation();
    const SDFOperation newOp = (currentOp == SDFOperation::addition) ? SDFOperation::substraction : SDFOperation::addition;
    _object->setOperation(newOp);
}

void
ToggleObjectOperationCommand::undo()
{
    run();
}
