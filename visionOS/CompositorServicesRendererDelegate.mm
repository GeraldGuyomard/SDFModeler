//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "CompositorServicesRendererDelegate.h"

CompositorServicesRendererDelegate::CompositorServicesRendererDelegate(cp_layer_renderer_t layerRenderer)
: _layerRenderer(layerRenderer)
{}

CompositorServicesRendererDelegate::~CompositorServicesRendererDelegate()
{
    //cp_release(_layerRenderer);
}

bool
CompositorServicesRendererDelegate::init(Renderer* renderer)
{
    
    return false;
}

RenderTargetConfiguration::CPtr
CompositorServicesRendererDelegate::configuration() const
{
    return _configuration;
}

id<MTLDevice> _Nonnull 
CompositorServicesRendererDelegate::getMTLDevice() const
{
    return nil;
}

float2
CompositorServicesRendererDelegate::renderSize() const
{
    return float2 { 0, 0 };
}

float2
CompositorServicesRendererDelegate::renderSizeInPoints() const
{
    return { 0, 0 };
}

MTLRenderPassDescriptor* _Nullable
CompositorServicesRendererDelegate::currentRenderPassDescriptor() const
{
    return nil;
}

id <MTLDrawable> _Nonnull
CompositorServicesRendererDelegate::currentDrawable() const
{
    return nil;
}

void
CompositorServicesRendererDelegate::invalidate()
{
    
}

void
CompositorServicesRendererDelegate::pause()
{
    
}
