//
//  XRDragInteraction.hpp
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 3/5/24.
//

#pragma once

#include "XR/XRHandAnchor.h"
#include "Object3D.h"
#include <memory>

class XRDragInteraction final
{
public:
    using Ptr = std::unique_ptr<XRDragInteraction>;
    
    XRDragInteraction(Chirality handChirality,
                      float3 initialDraggingPosInWorld,
                      JointID jointID,
                      const Object3D::Ptr& object);
    
    bool update(const XRHandAnchor* left, const XRHandAnchor* right);
    
private:
    const Chirality _handChirality;
    const JointID _jointID;
    const float3 _initialDraggingPosInWorld;
    const Object3D::Ptr _object;
    const float4x4 _initialObjectWorldTransform;
};

