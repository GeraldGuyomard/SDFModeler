//
//  XRDragInteraction.hpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#pragma once

#include "XR/XRHandAnchor.h"

#include "XRInteraction.h"
#include "Object3D.h"
#include "TransformObjectCommand.h"

#include <memory>
#include <array>

class XRDualPinchInteraction final : public XRInteraction
{
public:
    using Ptr = std::shared_ptr<XRDualPinchInteraction>;

    XRDualPinchInteraction(const WorldPtr& world);
    
    void update(const XRHandAnchors&) override;
    void commit() override;
    
private:
    
    WorldPtr _world;
    
    struct ActiveState final
    {
        const float initialDistance;
        const TransformObjectCommand::Entry entry;
        
        ActiveState(float dist, const Object3D::Ptr& object)
        : initialDistance(dist), entry { object }
        {}
    };
    
    std::unique_ptr<ActiveState> _activeState;
    
};

