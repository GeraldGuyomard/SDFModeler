//
//  XRDragInteraction.cpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#include "XRInteraction.h"


void
XRInteractionManager::process(const XRHandAnchors& anchors)
{
    XRInteraction::Ptr previousActiveInteraction;
    if (_activeInteraction != nullptr)
    {
        ASSERT(_activeInteraction->state() == XRInteraction::State::active);
        const auto requiredState = _activeInteraction->update(anchors);
        if (requiredState == XRInteraction::State::active)
        {
            // exclusive, one active interaction at a time
            return;
        }

        _activeInteraction->_setState(requiredState);
        previousActiveInteraction = _activeInteraction;
        _activeInteraction.reset();
    }
    
    for (const auto& interaction : _interactions)
    {
        if (interaction == previousActiveInteraction)
        {
            continue;
        }
        
        const auto requiredState = interaction->update(anchors);
        interaction->_setState(requiredState);
        
        if (requiredState == XRInteraction::State::active)
        {
            _activeInteraction = interaction;
            
            // deactivate the other interactions
            for (const auto& otherInteraction : _interactions)
            {
                if (otherInteraction != _activeInteraction)
                {
                    otherInteraction->_setState(XRInteraction::State::inactive);
                }
            }
            
            break;
        }
    }
}

void
XRInteractionManager::add(const XRInteraction::Ptr& interaction)
{
    _interactions.push_back(interaction);
    interaction->_setState(XRInteraction::State::inactive);
}

void
XRInteraction::_setState(State state)
{
    if (_state != state)
    {
        const auto oldState = _state;
        _state = state;
        _onStateChanged(oldState, _state);
        
        if (_stateChangedCallback != nullptr)
        {
            _stateChangedCallback(shared_from_this(), oldState, state);
        }
    }
}

void
XRInteraction::setStateChangedCallback(const StateChangedCallback& cb)
{
    _stateChangedCallback = cb;
}

