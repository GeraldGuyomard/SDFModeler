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
    std::optional<float3> worldTipPosition(const XRHandAnchor* anchor)
    {
        if (anchor == nullptr)
        {
            return std::nullopt;
        }
        
        const auto handTransform = anchor->worldTransform();
        const auto tipTransform = anchor->jointTransformInHandSpace(JointID::indexFingerTip);
        const auto worldTransform = handTransform * tipTransform;
        return translation(worldTransform);
    }

    struct Hand
    {
        const XRHandAnchor* const anchor;
        const std::optional<float3> position;
        
        Object3D::Ptr object;
        float distance = 1e10f;
        
        Hand(const XRHandAnchor* anchor)
        : anchor(anchor), position(worldTipPosition(anchor))
        {}
        
        void updateDistance(const Object3D::Ptr& o)
        {
            if (!position.has_value())
            {
                return;
            }
            
            const float d = o->computeDistance(position.value());
            if (d < distance)
            {
                distance = d;
                object = o;
            }
        }
    };

    void findClosestObject(const Object3D::Ptr& object, Hand& leftHand, Hand& rightHand)
    {
        leftHand.updateDistance(object);
        rightHand.updateDistance(object);
        
        for (const auto& child : object->children())
        {
            findClosestObject(child, leftHand, rightHand);
        }
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
        auto handAnchors = xrService.handAnchors();
        const XRHandAnchor* leftHand = nullptr;
        const XRHandAnchor* rightHand = nullptr;
        
        for (const auto& anchor : handAnchors)
        {
            if (anchor->chirality() == Chirality::left)
            {
                leftHand = anchor.get();
            }
            else
            {
                rightHand = anchor.get();
            }
        }
        
        onHandUpdate(leftHand, rightHand);
    }
    
    void onHandUpdate(const XRHandAnchor* leftHandAnchor, const XRHandAnchor* rightHandAnchor)
    {
        const bool leftHere = leftHandAnchor != nullptr;
        const bool rightHere = rightHandAnchor != nullptr;
        NSLog(@"left:%d right:%d", leftHere, rightHere);
        
        Hand leftHand { leftHandAnchor };
        Hand rightHand { rightHandAnchor };
        
        if (!leftHand.position.has_value() && !rightHand.position.has_value())
        {
            return;
        }
        
        // find if close to to an object
        findClosestObject(_world->rootObject(), leftHand, rightHand);
        
        const Hand* closestHand = nullptr;
        
        if (leftHand.position.has_value())
        {
            if (rightHand.position.has_value())
            {
                if (leftHand.distance < rightHand.distance)
                {
                    closestHand = &leftHand;
                }
                else
                {
                    closestHand = &rightHand;
                }
            }
            else
            {
                closestHand = &leftHand;
            }
        }
        else if (rightHand.position.has_value())
        {
            closestHand = &rightHand;
        }
        
        if (closestHand->distance <= 0.05f)
        {
            auto object = closestHand->object;
            NSLog(@"Close to object %d at distance %5.3fm", int(object->id()), closestHand->distance);
            _world->setSelection(object);
            
            if (closestHand->anchor->isPinching())
            {
                NSLog(@"Hand pinched, chirality:%d", int(closestHand->anchor->chirality()));
            }
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
