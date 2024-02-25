//
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MoveCameraAnimation.h"

MoveCameraAnimation::MoveCameraAnimation(const CameraRig::Ptr& camera, float duration, const float3& finalPos)
: _camera(camera), _duration(duration), _finalPos(finalPos), _startPos(translation(camera->worldTransform()))
{}

bool
MoveCameraAnimation::isFinished() const
{
    return _finished;
}

void
MoveCameraAnimation::start(float t)
{
    _startT = t;
}

void
MoveCameraAnimation::update(float time)
{
    const float dt = time - _startT;
    float t = dt / _duration;
    
    if (t > 1.f)
    {
        _finished = true;
    }
    else
    {
        t = t * t * (3.0f - 2.0f * t);
        
        const float3 pos = mix(_startPos, _finalPos, t);
        
        auto worldTransform = _camera->worldTransform();
        setTranslation(worldTransform, pos);
        _camera->setWorldTransform(worldTransform);
    }
}
