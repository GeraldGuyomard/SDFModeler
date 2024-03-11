//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"
#include "Object3D.h"
#include <memory>

@class XRHandTrackingImpl, XRHandAnchorImpl;

enum class Chirality : size_t
{
    left = 0,
    right = 1
};

static constexpr size_t kMaxChirality = 2;

enum class JointID
{
    thumbTip = 0,
    indexFingerTip = 1,
    middleFingerTip = 2,
    ringFingerTip = 3,
    littleFingerTip = 4,
    wrist = 5,
    
    jointCount
};

class XRHandAnchor final
{
public:
    using Ptr = std::shared_ptr<XRHandAnchor>;
    
    XRHandAnchor(XRHandAnchorImpl* _Nonnull impl);
    ~XRHandAnchor();
    
    Chirality chirality() const;
    bool isTracked() const;
    float4x4 worldTransform() const;
    
    float4x4 jointTransformInHandSpace(JointID) const;
    float4x4 jointTransformInWorldSpace(JointID) const;
    
    static constexpr float kDefaultFingerDistance = 0.01f;
    bool isPinching(float minDistance = kDefaultFingerDistance) const;
    
private:
    XRHandAnchorImpl* const _Nonnull _impl;
    
    // cached
    mutable std::optional<Chirality> _chirality;
    mutable std::optional<bool> _isTracked;
    mutable std::optional<float4x4> _worldTransform;
    
    mutable std::array<std::optional<float4x4>, size_t(JointID::jointCount)> _jointTransformInHandSpace;
    mutable std::array<std::optional<float4x4>, size_t(JointID::jointCount)> _jointTransformInWorldSpace;
    
    mutable std::optional<bool> _isPinching;
    
};

class XRHandAnchors final
{
public:
    XRHandAnchors();
    XRHandAnchors(const WorldPtr& world, const XRHandAnchor::Ptr& left, const XRHandAnchor::Ptr& right);
    
    const XRHandAnchor::Ptr& anchor(Chirality) const;
    const XRHandAnchor::Ptr& otherAnchor(Chirality) const;
    
    bool none() const;
    
    struct Entry final
    {
        XRHandAnchor::Ptr handAnchor;
        float3 position = { 0.f, 0.f, 0.f };
        Object3D::Ptr object;
        float distance = 1e10f;
    };
    
    const std::array<Entry, kMaxChirality>& entries() const { return _entries; }
    const Entry& entry(Chirality) const;
    
    const Entry* _Nullable closestEntryToAnyHand() const;
    
private:
    
    void _updateDistances(const Object3D::Ptr& o);
    
    std::array<Entry, kMaxChirality> _entries;
};

