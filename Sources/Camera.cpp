//
//  Camera.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "Camera.h"

void
Camera::setWorldTransform(const float4x4& t)
{
    _worldTransform = t;
}
