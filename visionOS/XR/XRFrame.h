//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"

#include <memory>

@class XRFrameImpl;

class XRFrame final
{
public:
    
    using Ptr = std::unique_ptr<XRFrame>;
    
    XRFrame(XRFrameImpl* _Nonnull impl);
    ~XRFrame();
    
    void startUpdate();
    void endUpdate();
    
    bool waitUntilOptimalTime();
    
    void startSubmission();
    void endSubmission();
    
    bool isValid() const
    {
        return _impl != nullptr;
    }
    
    XRFrameImpl* _Nullable impl() const
    {
        return _impl;
    }
    
private:
    XRFrameImpl* const _Nonnull _impl;
};
