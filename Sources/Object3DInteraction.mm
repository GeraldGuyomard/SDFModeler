//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Object3DInteraction.h"
#include "SDFPlane.h"

DragObject3DInteraction::DragObject3DInteraction(const WorldPtr& world,
                                                 const Object3D::Ptr& object,
                                                 const float3& hitPos3D,
                                                 const float2& initialPos,
                                                 const Renderer& renderer)
: PanInteraction(initialPos),
_world(world),
_object(object),
_hitPos3D(hitPos3D),
_renderer(renderer),
_initialTransform(object->worldTransform()),
_command(std::make_shared<TransformObjectCommand>(_object))
{
    _transform = _initialTransform;
    _world->commandHistory().enable(false);
}

DragObject3DInteraction::~DragObject3DInteraction() = default;

void
DragObject3DInteraction::pan(const float2& pos)
{
    const Ray ray = _renderer.ray(pos);
    
    SDFObject<SDFPlane> plane { SDFPlane {}, RSTTransformer { _hitPos3D } };
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
    _command->setTransform(_transform);
    _world->commandHistory().run(_command);
    _world->commandHistory().enable(true);
}
