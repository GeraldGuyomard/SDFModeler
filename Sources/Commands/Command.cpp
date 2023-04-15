//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Command.h"

CommandHistory::CommandHistory(size_t undoDepth)
: _undoDepth(undoDepth)
{}

void
CommandHistory::onStateChange()
{
    if (_stateUpdateCallback != nullptr)
    {
        _stateUpdateCallback(*this);
    }
}

bool
CommandHistory::isEnabled() const
{
    return _enableCount > 0;
}

void
CommandHistory::enable(bool enable)
{
    if (enable)
    {
        ++_enableCount;
    }
    else
    {
        --_enableCount;
    }
    
    onStateChange();
}

void
CommandHistory::run(const Command::Ptr& cmd)
{
    cmd->run();
    _undoableCommands.push_back(cmd);
    
    while (_undoableCommands.size() > _undoDepth)
    {
        _undoableCommands.erase(_undoableCommands.begin());
    }
    
    _redoableCommands.clear();
    
    onStateChange();
}

bool
CommandHistory::canUndo() const
{
    return isEnabled() && !_undoableCommands.empty();
}

void
CommandHistory::undo()
{
    if (canUndo())
    {
        auto cmd = _undoableCommands.back();
        _undoableCommands.erase(_undoableCommands.end() - 1);
        cmd->undo();
        _redoableCommands.push_back(cmd);
        
        onStateChange();
    }
}

bool
CommandHistory::canRedo() const
{
    return isEnabled() && !_redoableCommands.empty();
}

void
CommandHistory::redo()
{
    if (canRedo())
    {
        auto cmd = _redoableCommands.back();
        _redoableCommands.erase(_redoableCommands.end() - 1);
        cmd->redo();
        _undoableCommands.push_back(cmd);
        
        onStateChange();
    }
}

void
CommandHistory::setStateUpdateCallback(const StateUpdateCallback& cb)
{
    _stateUpdateCallback = cb;
}
