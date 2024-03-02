//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include "CommonDefinitions.h"
#import <ARKit/ARKit.h>

#include <memory>

class XRService final
{
public:
    using Ptr = std::shared_ptr<XRService>;

    static Ptr make();
    ~XRService();

    void start();
    
    bool canQueryDeviceAnchor() const;
    ar_device_anchor_t _Nonnull queryDeviceAnchor(CFTimeInterval time);
    
private:
    XRService();
    bool _init();
    
    ar_session_t _Nullable _arSession = nil;
    ar_world_tracking_provider_t _Nullable _worldTracking = nil;
};


