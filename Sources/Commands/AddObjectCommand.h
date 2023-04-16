//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Object3D.h"

class AddObjectCommand : public Command
{
public:
    
    AddObjectCommand(const Object3D::Ptr& parent,
                     const Object3DFactory::Ptr& factory);
    
    void run() override;
    void undo() override;
    
private:
    const Object3D::Ptr _parent;
    const Object3DFactory::Ptr _factory;
    
    Object3D::Ptr _object;
    Material3D::Ptr _material;
};
