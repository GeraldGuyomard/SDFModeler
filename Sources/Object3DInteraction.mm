//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Object3DInteraction.h"
#include "SDFPlane.h"
#include "Commands/TransformObjectCommand.h"

DragObject3DInteraction::DragObject3DInteraction(World& world,
                                                 const Object3D::Ptr& object,
                                                 const float3& hitPos3D,
                                                 const float2& initialPos,
                                                 const Renderer& renderer)
: PanInteraction(initialPos),
_world(world),
_object(object),
_hitPos3D(hitPos3D),
_renderer(renderer),
_initialTransform(object->worldTransform())
{
    _transform = _initialTransform;
}

void
DragObject3DInteraction::pan(const float2& pos)
{
    const Ray ray = _renderer.ray(pos);
    
    Plane plane { SDFPlane {}, RSTTransformer { _hitPos3D } };
    float d = plane.raycast(ray);
    
    const float3 p = ray.pt(d);
    
    _transform = _initialTransform;
    const float3 newPos = translation(_transform) + p - _hitPos3D;
    
    setTranslation(_transform, newPos);
    
    _object->setWorldTransform(_transform);
}

void
DragObject3DInteraction::commit()
{
    auto action = std::make_shared<TransformObjectCommand>(_object, _transform);
    _world.commandHistory().run(action);
}
