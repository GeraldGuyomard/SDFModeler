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
CommandHistory::run(const Command::Ptr& cmd)
{
    cmd->run();
    _undoableCommands.push_back(cmd);
    
    while (_undoableCommands.size() > _undoDepth)
    {
        _undoableCommands.erase(_undoableCommands.begin());
    }
}

bool
CommandHistory::canUndo() const
{
    return !_undoableCommands.empty();
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
    }
}

bool
CommandHistory::canRedo() const
{
    return !_redoableCommands.empty();
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
    }
}
