//
//  CameraController.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Object3DInteraction.h"
#include "SDFPlane.h"

DragObject3DInteraction::DragObject3DInteraction(const WorldPtr& world,
                                                 const Object3DSelection& selection,
                                                 const float3& hitPos3D,
                                                 const float2& initialPos,
                                                 const Renderer& renderer)
: PanInteraction(initialPos),
_world(world),
_hitPos3D(hitPos3D),
_renderer(renderer)
{
    for (const auto& object : selection.objects())
    {
        _initialState.emplace_back(object);
    }
    
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
    const float3 delta = p - _hitPos3D;
    
    for (const auto& entry: _initialState)
    {
        auto transform = entry.transform;
        const float3 newPos = translation(transform) + delta;
        
        setTranslation(transform, newPos);
        
        entry.object->setWorldTransform(transform);
    }
}

void
DragObject3DInteraction::commit()
{
    std::vector<TransformObjectCommand::Entry> newState;
    
    for (auto& entry : _initialState)
    {
        newState.emplace_back(entry.object);
    }
    
    auto command = std::make_shared<TransformObjectCommand>(_initialState);
    
    _world->commandHistory().run(command);
    _world->commandHistory().enable(true);
}
