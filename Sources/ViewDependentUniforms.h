//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CameraUniforms.h"
#include "SerializedWorldObject.h"

CONSTANT constexpr size_t kNbViews = 2;

struct ViewDependentUniforms final
{
    CameraUniforms cameraUniforms[kNbViews];
    SerializedWorldObject serializedWorldObject[kNbViews];
};
