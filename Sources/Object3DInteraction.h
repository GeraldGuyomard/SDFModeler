//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "Interaction.h"
#include "Object3D.h"
#include "Renderer.h"

class DragObject3DInteraction : public PanInteraction
{
public:
    using _inherited = PanInteraction;
    
    DragObject3DInteraction(const Object3D::Ptr& object, const float2& initialPos, const Renderer&);
    
    void pan(const float2& pos) override;
    
private:
    const Object3D::Ptr _object;
};
