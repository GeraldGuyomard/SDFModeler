//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Object3D.h"

class TransformObjectCommand : public Command
{
public:
    using Ptr = std::shared_ptr<TransformObjectCommand>;
    
    struct Entry final
    {
        Object3D::Ptr object;
        float4x4 transform;
        
        Entry(const Object3D::Ptr& object);
    };
    
    TransformObjectCommand(const std::vector<Entry>& initialState);
    
    void run() override {}
    void undo() override { undoRedo(); }
    void redo() override { undoRedo(); }
    
private:
    
    void undoRedo();
    
    std::vector<Entry> _state;
};
