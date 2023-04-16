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

#include "Commands/TransformObjectCommand.h"

class World;

class DragObject3DInteraction : public PanInteraction
{
public:
    using _inherited = PanInteraction;
    
    DragObject3DInteraction(const WorldPtr& world,
                            const Object3D::Ptr& object,
                            const float3& hitPos3D,
                            const float2& initialPos,
                            const Renderer&);
    ~DragObject3DInteraction() override;
    
    void pan(const float2& pos) override;
    void commit() override;
    
private:
    const WorldPtr _world;
    
    const Object3D::Ptr _object;
    const float3 _hitPos3D;
    const Renderer& _renderer;
    const float4x4 _initialTransform;
    float4x4 _transform;
    
    const std::shared_ptr<class TransformObjectCommand> _command;
};
