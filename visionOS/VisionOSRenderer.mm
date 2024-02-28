//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "VisionOSRenderer.h"
#include "CompositorServicesRendererDelegate.h"
#include "WorldHelpers.h"

@implementation VisionOSRenderer
{
    WorldPtr _world;
    std::unique_ptr<Renderer> _renderer;
}

- (instancetype) initWithLayerRenderer:(cp_layer_renderer_t)layerRenderer
{
    if (self = [self init])
    {
        constexpr float s = 1.f;
        float4x4 transform = matrix4x4_scale(s);
        setTranslation(transform, float3 {0, 0, -5.f});
        
        _world = makeDefaultWorld(transform);
        
        auto delegate = std::make_unique<CompositorServicesRendererDelegate>(layerRenderer);
        _renderer = std::make_unique<Renderer>(_world, std::move(delegate));
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

- (void)renderImage
{
    auto image = _renderer->renderImage();
    int a;
    a = 1;
}

+(void) renderOnCPU
{
    if (s_Instance != nil)
    {
        [s_Instance renderImage];
    }
}

@end
