//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include <memory>
#include <vector>
#include <functional>

class Command
{
public:
    using Ptr = std::shared_ptr<Command>;
    
    virtual ~Command() = default;
    
    virtual void run() = 0;
    virtual void undo() = 0;
    virtual void redo() { run(); }
};

class SymetricalCommand : public Command
{
public:
    void run() override { execute(); }
    void undo() override { execute(); }
    void redo() override { execute(); }
    
protected:
    virtual void execute() = 0;
};

class CommandHistory final
{
public:
    
    static constexpr size_t kDefaultUndoDepth = 20;
    
    CommandHistory(size_t undoDepth = kDefaultUndoDepth);
    
    bool isEnabled() const;
    void enable(bool);
    
    void run(const Command::Ptr&);
    
    bool canUndo() const;
    void undo();
    
    bool canRedo() const;
    void redo();
    
    using StateUpdateCallback = std::function<void(const CommandHistory&)>;
    void setStateUpdateCallback(const StateUpdateCallback&);
    
private:
                                              
    void onStateChange();
                                              
    std::vector<Command::Ptr> _redoableCommands;
    std::vector<Command::Ptr> _undoableCommands;
    
    int32_t _enableCount = 1;
    size_t _undoDepth;
    StateUpdateCallback _stateUpdateCallback;
};
