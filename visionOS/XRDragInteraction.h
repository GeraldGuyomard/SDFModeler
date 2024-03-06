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

class XRDragInteraction final
{
public:
    using Ptr = std::shared_ptr<XRDragInteraction>;
    
    XRDragInteraction(Chirality handChirality,
                      float3 initialDraggingPosInWorld,
                      JointID jointID,
                      const Object3D::Ptr& object);
    
    bool update(const XRHandAnchor* left, const XRHandAnchor* right);
    void commit();
    
private:
    const Chirality _handChirality;
    const JointID _jointID;
    const float3 _initialDraggingPosInWorld;
    
    const TransformObjectCommand::Entry _entry;
};

