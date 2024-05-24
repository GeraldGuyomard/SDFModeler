//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "SetPropertyCommand.h"

SetPropertyCommand::SetPropertyCommand(void* object, const Property::Ptr& property, Renderer* renderer)
: _object(object), _property(property), _savedValue(property->get(object)), _renderer(renderer)
{}

void
SetPropertyCommand::setValue(const TPropertyValue& v)
{
    _property->set(_object, v);
    _renderer->invalidate();
}

void
SetPropertyCommand::run()
{}

void
SetPropertyCommand::undo()
{
    _swapAndSet();
}

void
SetPropertyCommand::redo()
{
    _swapAndSet();
}

void
SetPropertyCommand::_swapAndSet()
{
    auto current = _property->get(_object);
    std::swap(current, _savedValue);
    
    setValue(current);
}
