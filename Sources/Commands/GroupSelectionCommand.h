//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Object3D.h"

class GroupSelectionCommand : public Command
{
public:
    GroupSelectionCommand(const Object3DSelection&);
    
    void run() override;
    void undo() override;
    
private:
    
    struct Entry final
    {
        const Object3D::Ptr object;
        const Object3D::Ptr parent;
        const ObjectID id;
        
        Entry(const Object3D::Ptr&);
    };
    
    std::vector<Entry> _entries;
    Object3D::Ptr _group;
};
