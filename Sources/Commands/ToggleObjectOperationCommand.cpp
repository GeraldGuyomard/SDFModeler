//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "ToggleObjectOperationCommand.h"

ToggleObjectOperationCommand::ToggleObjectOperationCommand(const Object3DSelection& selection)
: _selection(selection)
{
}

void
ToggleObjectOperationCommand::run()
{
    for (const auto& object: _selection.objects())
    {
        const auto currentOp = object->operation();
        const SDFOperation newOp = (currentOp == SDFOperation::addition) ? SDFOperation::substraction : SDFOperation::addition;
        object->setOperation(newOp);
    }
}

void
ToggleObjectOperationCommand::undo()
{
    run();
}
