//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "CameraRig.h"

LookAtObject3DProvider::LookAtObject3DProvider(const Object3D::Ptr& object)
: _object(object)
{}

std::optional<float3>
LookAtObject3DProvider::position() const
{
    if (auto object = _object.lock())
    {
        return translation(object->worldTransform());
    }
    else
    {
        return {};
    }
}

CameraRig::CameraRig(const WorldPtr& world)
: _inherited(world)
{
}

CameraRig::Ptr
CameraRig::make(const WorldPtr& world, size_t nbCameras)
{
    Ptr ptr { new CameraRig(world) };
    
    ptr->_cameras.resize(nbCameras);
    
    for (auto& camera : ptr->_cameras)
    {
        camera = std::make_shared<Camera>(world);
        ptr->addChild(camera);
    }
    
    return ptr;
}

float3
CameraRig::lookAtPosition()
{
    std::optional<float3> pos;
    if (_lookAtPositionProvider != nullptr)
    {
        pos = _lookAtPositionProvider->position();
    }
    
    if (pos.has_value())
    {
        _defaultLookAtPosition = pos.value();
    }
    
    return _defaultLookAtPosition;
}

void
CameraRig::setLookAtPositionProvider(LookAtPositionProvider::Ptr provider)
{
    _lookAtPositionProvider = std::move(provider);
}

void
CameraRig::setLookAtPositionProvider(const Object3D::Ptr& object)
{
    if (object != nullptr)
    {
        _lookAtPositionProvider = std::make_unique<LookAtObject3DProvider>(object);
    }
    else
    {
        _lookAtPositionProvider.reset();
    }
}

float3
CameraRig::computeOrbitOrigin()
{
    const auto cameraTransform = worldTransform();
    
    const auto position = cameraTransform.columns[3].xyz;
    const auto direction = simd_normalize(cameraTransform.columns[2].xyz);
    
    const float3 lookAtVector = lookAtPosition() - position;
    const float proj = dot(direction, lookAtVector);
    return position + (direction * proj);
}
