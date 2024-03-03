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

float3
CameraRig::computeFramePosition(const Object3D::Ptr& object) const
{
    // work with left eye
    const auto& camera = _cameras.front();
    const auto intrinsics = camera->intrinsics();
    if (intrinsics == nullptr)
    {
        ASSERT(false);
        return 0.f;
    }
    
    const auto worldBox = object->worldBoundingBoxOfHierarchy();
    const auto worldOrigin = worldBox.center();
    
    // Align first camera to face origin of object with arbitrary distance
    auto worldTransform = this->worldTransform();
    auto startPos = worldOrigin + forward(worldTransform);
    setTranslation(worldTransform, startPos);
    
    auto coordinateSpace = inverse(worldTransform);
    const auto box = object->boundingBoxOfHierarchyInCoordinateFrame(coordinateSpace);
    
    // assume fov is same for all cameras
    const auto fov = intrinsics->fovRadians(camera->viewportSize());
    const float tanX = tanf(fov.x / 2.f);
    const float tanY = tanf(fov.y / 2.f);
    
    // tan(f/2) = halfSize / d
    // -> d = halfSize / tan(f/2)
    
    const float xMin = box.minPoint.x;
    const float dXMin = fabsf(xMin) / tanX;
    
    const float xMax = box.maxPoint.x;
    const float dXMax = fabsf(xMax) / tanX;

    const float yMin = box.minPoint.y;
    const float dYMin = fabsf(yMin) / tanY;
    
    const float yMax = box.maxPoint.y;
    const float dYMax = fabsf(yMax) / tanY;
    
    const float d = max(dXMin, max(dXMax, max(dYMin, dYMax)));
    
    const float4 pt { 0, 0, d + box.maxPoint.z, 1.f };
    
    const auto pos = worldTransform * pt;
    
    return pos.xyz;
}

float4x4
CameraRig::computeFrameTransform(const Object3D::Ptr& object) const
{
    auto transform = worldTransform();
    setTranslation(transform, computeFramePosition(object));
    return transform;
}
