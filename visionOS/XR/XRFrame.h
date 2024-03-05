//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"
#import <ARKit/ARKit.h>
#import <CompositorServices/CompositorServices.h>

#include <memory>

@class XRFrameImpl;

class XRFrame final
{
public:
    
    XRFrame() = default;
    XRFrame(XRFrameImpl*_Nullable);
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
    
    void invalidate()
    {
        _impl = nil;
    }
    
private:
    XRFrameImpl* _Nonnull _impl = nil;
};
