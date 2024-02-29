//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRService.h"

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
    _arSession = ar_session_create();
    if (_arSession == nullptr)
    {
        return false;
    }
    
    auto config = ar_world_tracking_configuration_create();
    _worldTracking = ar_world_tracking_provider_create(config);
    if (_worldTracking == nullptr)
    {
        return false;
    }
    
    return true;
}

void
XRService::start()
{
    auto providers = ar_data_providers_create();
    ar_data_providers_add_data_provider(providers, _worldTracking);
    
    ar_session_run(_arSession, providers);
}

ar_device_anchor_t
XRService::queryDeviceAnchor(CFTimeInterval time)
{
    auto deviceAnchor = ar_device_anchor_create();
    const auto status = ar_world_tracking_provider_query_device_anchor_at_timestamp(_worldTracking, time, deviceAnchor);
    if (status != ar_device_anchor_query_status_success)
    {
        return nullptr;
    }
    
    return deviceAnchor;
}
