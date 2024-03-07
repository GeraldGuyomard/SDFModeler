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

#include "XRFrame.h"
#include "XRDrawable.h"
#include "XRHandAnchor.h"

@class XRServiceImpl;

class XRService final
{
public:
    using Ptr = std::shared_ptr<XRService>;

    static Ptr make();
    ~XRService();

    using Completion = std::function<void()>;
    void start(const Completion&);
    
    XRFrame::Ptr queryNextFrame(cp_layer_renderer_t _Nonnull layerRenderer);
    
    XRDrawable::Ptr queryDrawable(const XRFrame& frame);
    
    // tracking
    float4x4 worldHeadTransform(const XRDrawable&) const;
    
    XRHandAnchors handAnchors() const;
    
private:
    XRService();
    bool _init();
    
    XRServiceImpl* _Nonnull _impl = nil;
};


