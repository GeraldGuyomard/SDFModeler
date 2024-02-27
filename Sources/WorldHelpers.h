//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Object3D.h"

WorldPtr makeDefaultWorld(const float4x4& worldTransform = float4x4_identity());
