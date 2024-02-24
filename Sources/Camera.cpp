//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Camera.h"

Camera::Camera(const WorldPtr& world)
: _inherited(world)
{}

void
Camera::setViewportSize(const float2& size)
{
    _viewportSize = size;
}

float
Camera::aspectRatio() const
{
    return _viewportSize.x / _viewportSize.y;
}

float4x4
Camera::computeProjectionMatrix() const
{
    return matrix_perspective_right_hand(_fovyRadians, aspectRatio(), _nearZ, _farZ);
}

float
Camera::fovxRadians() const
{
    return 2.f * atanf(tanf(_fovyRadians / 2.f)) * aspectRatio();
}

float3
Camera::computeFramePosition(const Object3D::Ptr& object) const
{
    const auto worldBox = object->worldBoundingBoxOfHierarchy();
    const auto worldOrigin = worldBox.center();
    
    // Align first camera to face origin of object with arbitrary distance
    auto worldTransform = this->worldTransform();
    auto startPos = worldOrigin + forward(worldTransform);
    setTranslation(worldTransform, startPos);
    
    auto coordinateSpace = inverse(worldTransform);
    const auto box = object->boundingBoxOfHierarchyInCoordinateFrame(coordinateSpace);
    
    const auto fov = fovRadians();
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
Camera::computeFrameTransform(const Object3D::Ptr& object) const
{
    auto transform = worldTransform();
    setTranslation(transform, computeFramePosition(object));
    return transform;
}
