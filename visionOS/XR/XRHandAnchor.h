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

enum class Chirality
{
    left,
    right
};

enum class JointID
{
    thumbTip,
    indexFingerTip,
    middleFingerTip,
    ringFingerTip,
    littleFingerTip,
};

class XRHandAnchor final
{
public:
    using Ptr = std::unique_ptr<XRHandAnchor>;
    
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

