//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Object3D.h"

class RemoveObjectCommand : public Command
{
public:
    using Ptr = std::shared_ptr<RemoveObjectCommand>;
    
    RemoveObjectCommand(World* world, const Object3D::Ptr& object);
    
    void run() override;
    void undo() override;
    
private:
    World* const _world;
    const Object3D::Ptr _parent;
    const Object3D::Ptr _object;
};
