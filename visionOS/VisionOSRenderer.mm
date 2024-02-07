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
    RendererDelegate::Ptr _delegate;
}

- (instancetype) initWithLayerRenderer:(cp_layer_renderer_t)renderer
{
    if (self = [self init])
    {
        _delegate = std::make_unique<CompositorServicesRendererDelegate>(renderer);
    }
    
    return self;
}

- (void)dealloc
{
    
}

- (void)startRenderLoop
{
    
}

@end
