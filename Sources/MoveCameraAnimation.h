//
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "Animation.h"
#include "Camera.h"

class MoveCameraAnimation : public Animation
{
public:
    MoveCameraAnimation(const Camera::Ptr& camera, float duration, const float3& finalPos);
    
    bool isFinished() const override;
    void start(float t) override;
    void update(float t) override;
    
private:
    const Camera::Ptr _camera;
    const float _duration;
    const float3 _finalPos;
    float3 _startPos;
    
    bool _finished = false;
    float _startT = 0.f;
};
