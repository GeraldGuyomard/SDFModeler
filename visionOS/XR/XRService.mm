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

XRHandAnchors
XRService::handAnchors() const
{
    const auto anchorsImpl = [_impl handAnchors];
    if (anchorsImpl == nil)
    {
        return {};
    }
    
    XRHandAnchors anchors;
    if (auto leftImpl = anchorsImpl.left)
    {
        anchors.anchor(Chirality::left) = std::make_shared<XRHandAnchor>(leftImpl);
    }

    if (auto rightImpl = anchorsImpl.right)
    {
        anchors.anchor(Chirality::right) = std::make_shared<XRHandAnchor>(rightImpl);
    }
    
    return anchors;
}

