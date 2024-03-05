//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRService.h"
#import "SDFModeler_visionOS-Swift.h"

XRService::XRService()
{}

XRService::~XRService()
{
}

XRService::Ptr
XRService::make()
{
    Ptr service { new XRService };
    if (!service->_init())
    {
        return nullptr;
    }
    
    return service;
}

bool
XRService::_init()
{
    _impl = [XRServiceImpl new];
    
    return true;
}

void
XRService::start(const Completion& completion)
{
    const auto completionCopy = completion;
    
    [_impl startWithCompletion:^{
        completionCopy();
    }];
}

XRFrame::Ptr
XRService::queryNextFrame(cp_layer_renderer_t layerRenderer)
{
    auto impl = [_impl queryNextFrameWithLayerRenderer:layerRenderer];
    if (impl == nil)
    {
        return nullptr;
    }
    
    return std::make_unique<XRFrame>(impl);
}

XRDrawable::Ptr
XRService::queryDrawable(const XRFrame& frame)
{
    XRDrawableImpl* impl =  [_impl queryDrawableWithFrame:frame.impl()];
    if (impl == nullptr)
    {
        return nullptr;
    }
    return std::make_unique<XRDrawable>(impl);
}

float4x4
XRService::worldHeadTransform(const XRDrawable& d) const
{
    return [_impl worldHeadTransform:d.impl()];
}

XRHandTracking::Ptr
XRService::latestHandTracking() const
{
    const auto impl = [_impl latestHandTracking];
    if (impl == nil)
    {
        return nullptr;
    }
    
    return std::make_unique<XRHandTracking>(impl);
}
