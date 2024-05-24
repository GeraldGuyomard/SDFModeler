//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Type.h"
#include "Renderer.h"

class SetPropertyCommand : public Command
{
public:
    using Ptr = std::shared_ptr<SetPropertyCommand>;
    
    SetPropertyCommand(void* object, const Property::Ptr& property, Renderer*);
    
    void setValue(const TPropertyValue&);
    
    void run() override;
    void undo() override;
    void redo() override;
    
private:
    
    void _swapAndSet();
    
    void* _object;
    const Property::Ptr _property;
    Renderer* _renderer;
    
    TPropertyValue _savedValue;
};
