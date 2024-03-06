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

#include <memory>
#include <optional>
#include <chrono>

class XRUndoInteraction final
{
public:
    using Ptr = std::shared_ptr<XRUndoInteraction>;
    
    XRUndoInteraction(World&);
    
    void update(const XRHandAnchor* left, const XRHandAnchor* right);

private:
    World& _world;
    
    class Tracking final
    {
    public:
        Tracking(Chirality);
        
        Chirality chirality() const { return _chirality; }
        
        bool enoughTimeElapsed() const;
        
    private:
        Chirality _chirality;
        
        using Clock = std::chrono::system_clock;
        std::chrono::time_point<Clock> _startTime;
    };
    
    std::optional<Tracking> _tracking;
};

