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
    using Ptr = std::shared_ptr<TransformObjectCommand>;
    
    TransformObjectCommand(const Object3D::Ptr& object);
    
    void setTransform(const float4x4& transform);
    
    void run() override;
    void undo() override;
    
private:
    const Object3D::Ptr _object;
    const float4x4 _initialObjectTransform;
    float4x4 _transform;
};
