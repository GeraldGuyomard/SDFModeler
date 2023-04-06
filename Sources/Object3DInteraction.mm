//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Object3DInteraction.h"
#include "SDFPlane.h"

DragObject3DInteraction::DragObject3DInteraction(const Object3D::Ptr& object,
                                                 const float2& initialPos,
                                                 const Renderer& renderer)
: PanInteraction(initialPos), _object(object)
{
    const Ray ray = renderer.ray(initialPos);
    
}

void
DragObject3DInteraction::pan(const float2& pos)
{
    
}
