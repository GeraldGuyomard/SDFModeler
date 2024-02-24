//
//  Camera.hpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include <memory>
#include "CommonDefinitions.h"
#include "Object3D.h"
#include "Camera.h"

class LookAtPositionProvider
{
public:
    using Ptr = std::unique_ptr<LookAtPositionProvider>;
    virtual ~LookAtPositionProvider() = default;
    
    virtual std::optional<float3> position() const = 0;
};

class LookAtObject3DProvider : public LookAtPositionProvider
{
public:
    LookAtObject3DProvider(const Object3D::Ptr& object);
    
    std::optional<float3> position() const override;
    
private:
    Object3D::WPtr _object;
};

class CameraRig final : public Object3D
{
public:
    using Ptr = std::shared_ptr<CameraRig>;
    using _inherited = Object3D;
    
    static Ptr make(const WorldPtr& world, size_t nbCameras);
    
    const std::vector<Camera::Ptr>& cameras() const { return _cameras; }
    
    float3 lookAtPosition();
    void setLookAtPositionProvider(LookAtPositionProvider::Ptr);
    void setLookAtPositionProvider(const Object3D::Ptr&);
    
    float3 computeOrbitOrigin();
    
private:
    
    CameraRig(const WorldPtr& world);
    
    std::vector<Camera::Ptr> _cameras;
    
    LookAtPositionProvider::Ptr _lookAtPositionProvider;
    float3 _defaultLookAtPosition = float3 { 0.f };
};
