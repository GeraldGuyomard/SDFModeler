//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "VisionOSRenderer.h"
#include "CompositorServicesRendererDelegate.h"

@implementation VisionOSRenderer
{
    std::unique_ptr<Renderer> _renderer;
}

- (instancetype) initWithLayerRenderer:(cp_layer_renderer_t)layerRenderer
{
    if (self = [self init])
    {
        auto delegate = std::make_unique<CompositorServicesRendererDelegate>(layerRenderer);
        _renderer = std::make_unique<Renderer>(std::move(delegate));
    }
    
    return self;
}

static VisionOSRenderer* s_Instance = nil;

- (void)startRenderLoop
{
    s_Instance = self;
    
    auto delegate = static_cast<CompositorServicesRendererDelegate*>(_renderer->delegate());
    delegate->startRenderLoop();
}

@end
