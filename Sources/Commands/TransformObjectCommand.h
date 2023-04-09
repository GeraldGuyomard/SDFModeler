//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Command.h"
#include "Object3D.h"

class TransformObjectCommand : public Command
{
public:
    TransformObjectCommand(const Object3D::Ptr& object, const float4x4& transform);
    
    void run() override;
    void undo() override;
    
private:
    const Object3D::Ptr _object;
    float4x4 _transform;
};
