//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "CameraInteraction.h"
#import <UIKit/UIKit.h>
#include <set>

class MultiTouchCameraInteraction : public Interaction, public CameraInteraction
{
public:
    MultiTouchCameraInteraction(const Camera::Ptr&);
    
    void touchesBegan(UITouch* touch, const float2& pos);
    void touchesMoved(UITouch* touch, const float2& pos);
    void touchesEnded(UITouch* touch, const float2& pos);
    
private:
    std::set<UITouch*> _trackedTouches;
};
