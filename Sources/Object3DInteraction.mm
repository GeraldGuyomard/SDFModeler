//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Object3DInteraction.h"
#include "SDFPlane.h"

DragObject3DInteraction::DragObject3DInteraction(const Object3D::Ptr& object,
                                                 const float3& hitPos3D,
                                                 const float2& initialPos,
                                                 const Renderer& renderer)
: PanInteraction(initialPos),
_object(object),
_hitPos3D(hitPos3D),
_renderer(renderer),
_initialTransform(object->transform())
{}

void
DragObject3DInteraction::pan(const float2& pos)
{
    const Ray ray = _renderer.ray(pos);
    
    SDFPlane plane { _hitPos3D };
    float d = plane.raycast(ray);
    
    const float3 p = ray.pt(d);
    
    auto transform = _initialTransform;
    const float3 newPos = translation(transform) + p - _hitPos3D;
    
    setTranslation(transform, newPos);
    
    _object->setTransform(transform);
}
