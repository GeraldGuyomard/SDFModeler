//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Object3D.h"

class ToggleObjectOperationCommand : public Command
{
public:
    using Ptr = std::shared_ptr<ToggleObjectOperationCommand>;
    
    ToggleObjectOperationCommand(const Object3DSelection& selection);
    
    void run() override;
    void undo() override;
    
private:
    
    const Object3DSelection _selection;
};
