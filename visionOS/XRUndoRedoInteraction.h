//
//  XRDragInteraction.hpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#pragma once

#include "XR/XRHandAnchor.h"
#include "Object3D.h"
#include "TransformObjectCommand.h"
#include "XRInteraction.h"

#include <memory>
#include <optional>
#include <chrono>

class XRUndoRedoInteraction final : public XRInteraction
{
public:
    using Ptr = std::shared_ptr<XRUndoRedoInteraction>;
    
    enum class Type
    {
        undo,
        redo
    };
    
    XRUndoRedoInteraction(const WorldPtr& world, Type);
    
    State update(const XRHandAnchors&) override;

protected:
    void _onStateChanged(State oldState, State newState) override;
                           
private:
    WorldPtr _world;
    const Type _type;
    
    State _updateWhenInactive(const XRHandAnchors&);
    State _updateWhenPossible(const XRHandAnchors&);
    State _updateWhenActive(const XRHandAnchors&);
    
    bool _isGestureDetected(const XRHandAnchor& anchor) const;
    
    class Tracking final
    {
    public:
        Tracking(Chirality);
        
        Chirality chirality() const { return _chirality; }
        
        bool enoughTimeElapsed() const;
        void resetTime();
        
    private:
        Chirality _chirality;
        
        using Clock = std::chrono::system_clock;
        std::chrono::time_point<Clock> _startTime;
    };
    
    std::optional<Tracking> _tracking;
};

