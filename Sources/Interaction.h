//
//  CameraController.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "Camera.h"

class Interaction
{
public:
    using Ptr = std::shared_ptr<Interaction>;
    
    virtual ~Interaction() = default;
};

class PanInteraction : public Interaction
{
public:
    using _inherited = Interaction;
    
    PanInteraction(const float2& initialPos);
    
    const float2& initialPos() const { return _initialPos; }
    
    virtual void pan(const float2& pos) = 0;
    
private:
    const float2 _initialPos;
};

class PinchInteraction : public Interaction
{
public:
    virtual void pinch(float delta) = 0;
};
