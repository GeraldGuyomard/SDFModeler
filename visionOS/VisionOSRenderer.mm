//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "VisionOSRenderer.h"
#include "CompositorServicesRendererDelegate.h"
#include "WorldHelpers.h"
#include "XRService.h"

@implementation VisionOSRenderer
{
    WorldPtr _world;
    
    XRService::Ptr _xrService;
    std::unique_ptr<Renderer> _renderer;
}

static __weak VisionOSRenderer* s_Instance = nil;

- (instancetype) initWithLayerRenderer:(cp_layer_renderer_t)layerRenderer
{
    if (self = [self init])
    {
        ASSERT(s_Instance == nil);
        s_Instance = self;
        
        constexpr float s = 0.25f;
        float4x4 transform = matrix4x4_scale(s);
        setTranslation(transform, float3 {0, 1.f, -1.f});
        
        _world = makeDefaultWorld(transform);
        
        /*if (auto object = _world->rootObject()->objectByID(ObjectID {6}))
        {
            _world->setSelection(object);
        }*/
        
        _xrService = XRService::make();
        
        auto delegate = std::make_unique<CompositorServicesRendererDelegate>(layerRenderer, _xrService);
        _renderer = std::make_unique<Renderer>(_world, std::move(delegate));
    }
    
    return self;
}

- (void)dealloc
{
    
}

- (void)shutdown
{
    auto delegate = static_cast<CompositorServicesRendererDelegate*>(_renderer->delegate());
    delegate->shutdown();
    _renderer.reset();
}

- (void)startRenderLoop
{
    __strong auto myself = self;
    
    auto delegate = static_cast<CompositorServicesRendererDelegate*>(_renderer->delegate());
    
    _xrService->start([myself, delegate]{
        
        delegate->startRenderLoop([myself]
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                // normally myself will be destroyed
                // by the end of this completion
                [myself shutdown];
            });
            
        });
    });
    
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
