//
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#include "CommonDefinitions.h"
#include <memory>

class Animation
{
public:
    using Ptr = std::shared_ptr<Animation>;
    virtual ~Animation() = default;
    
    virtual bool isFinished() const = 0;
    virtual void start(float t) = 0;
    virtual void update(float t) = 0;
};
