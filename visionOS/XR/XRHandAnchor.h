//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"

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
    thumbTip,
    indexFingerTip,
    middleFingerTip,
    ringFingerTip,
    littleFingerTip,
    wrist
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
};

class XRHandAnchors
{
public:
    std::array<XRHandAnchor::Ptr, kMaxChirality> anchors;
    
    const XRHandAnchor::Ptr& anchor(Chirality) const;
    XRHandAnchor::Ptr& anchor(Chirality);
    
    const XRHandAnchor::Ptr& otherAnchor(Chirality) const;
    XRHandAnchor::Ptr& otherAnchor(Chirality);
    
    bool none() const;
};
