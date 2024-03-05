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

namespace
{
    std::pair<Object3D::Ptr, float /*distance*/> findClosestObject(const Object3D::Ptr& object, const float3& pos)
    {
        const float d = object->computeDistance(pos);
        
        std::pair<Object3D::Ptr, float> pair { object, d };
        
        for (const auto& child : object->children())
        {
            const auto childPair = findClosestObject(child, pos);
            if (childPair.second < pair.second)
            {
                pair = childPair;
            }
        }
        
        return pair;
    }
}

class App final
{
public:
    App()
    {
        constexpr float s = 0.25f;
        float4x4 transform = matrix4x4_scale(s);
        setTranslation(transform, float3 {0, 1.f, -1.f});
        
        _world = makeDefaultWorld(transform);
        
        /*if (auto object = _world->rootObject()->objectByID(ObjectID {6}))
        {
            _world->setSelection(object);
        }*/
    }
    
    const WorldPtr& world() const { return _world; }
    
    void updateLogic(Renderer& renderer, const XRService& xrService)
    {
        auto handTracking = xrService.latestHandTracking();
        if (handTracking != nullptr)
        {
            auto leftHand = handTracking->leftHand();
            if (leftHand != nullptr)
            {
                onHandUpdate(*leftHand);
            }

            auto rightHand = handTracking->rightHand();
            if (rightHand != nullptr)
            {
                onHandUpdate(*rightHand);
            }
        }
    }
    
    void onHandUpdate(const XRHandAnchor& handAnchor)
    {
        const auto pos = translation(handAnchor.worldTransform());
        NSLog(@"Hand pos x=%5.2f, y=%5.2f, z=%5.2f", pos.x, pos.y, pos.z);
        
        // find if close to to an object
        const auto pair = findClosestObject(_world->rootObject(), pos);
        const float d = pair.second;
        
        if (d <= 0.2f)
        {
            auto object = pair.first;
            NSLog(@"Close to object %d at distance %5.3fm", int(object->id()), d);
            _world->setSelection(object);
        }
        else
        {
            _world->setSelection({});
        }
    }
    
private:
    
    WorldPtr _world;
};

@implementation VisionOSRenderer
{
    std::unique_ptr<App> _app;
    
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
        
        _app = std::make_unique<App>();
        
        _xrService = XRService::make();
        
        auto delegate = std::make_unique<CompositorServicesRendererDelegate>(layerRenderer, _xrService);
        _renderer = std::make_unique<Renderer>(_app->world(), std::move(delegate));
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
    auto delegate = static_cast<CompositorServicesRendererDelegate*>(_renderer->delegate());
    
    auto app = _app.get();
    
    delegate->setUpdateLogicCallback([app](auto& renderer, auto& xrService)
    {
        app->updateLogic(renderer, xrService);
    });
    
    __strong auto myself = self;
    
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
