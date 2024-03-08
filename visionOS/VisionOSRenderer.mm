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
#include "SelectionOutlineRenderPass.h"

#include "XRDragInteraction.h"
#include "XRUndoRedoInteraction.h"
#include "XRDualPinchInteraction.h"

class App final
{
public:
    App();
    
    const WorldPtr& world() const { return _world; }
    
    void prepare(Renderer& renderer);
    
    void updateLogic(Renderer& renderer, const XRService& xrService);
    void onHandUpdate(Renderer& renderer, const XRHandAnchors& anchors);

private:
    void updateSelection(Renderer& renderer, const XRHandAnchors& anchors);
    void onInteractionStateChanged(const XRInteraction::Ptr&);
    
    void setOutlineColor(float4 c);
    
    WorldPtr _world;
    std::vector<SelectionOutlineRenderPass*> _outlinePasses;
    float4 _defaultOutlineColor;
    
    XRInteractionManager _interactionManager;
    XRDragInteraction::Ptr _dragInteraction;
    XRDualPinchInteraction::Ptr _scaleInteraction;
};

App::App()
{
    constexpr float s = 0.25f;
    float4x4 transform = matrix4x4_scale(s);
    setTranslation(transform, float3 {0, 1.f, -1.f});
    
    _world = makeDefaultWorld(transform);
    
    _scaleInteraction = std::make_shared<XRDualPinchInteraction>(_world);
    _scaleInteraction->setName("scale");
    _interactionManager.add(_scaleInteraction);
    
    _dragInteraction = std::make_shared<XRDragInteraction>(_world);
    _dragInteraction->setName("drag");
    _dragInteraction->setStateChangedCallback([this](const XRInteraction::Ptr& interaction, XRInteraction::State oldState, XRInteraction::State newState)
    {
        onInteractionStateChanged(interaction);
    });
    _interactionManager.add(_dragInteraction);
    
    auto undoInteraction = std::make_shared<XRDragInteraction>(_world);
    undoInteraction->setName("undo");
    _interactionManager.add(undoInteraction);
    
    auto redoInteraction = std::make_shared<XRUndoRedoInteraction>(_world, XRUndoRedoInteraction::Type::redo);
    redoInteraction->setName("redo");
    _interactionManager.add(redoInteraction);
}
    
    
    
void
App::prepare(Renderer& renderer)
{
    for (auto* pass : renderer.renderPasses())
    {
        if (auto* outlinePass = dynamic_cast<SelectionOutlineRenderPass*>(pass))
        {
            _outlinePasses.push_back(outlinePass);
        }
    }
    
    ASSERT(!_outlinePasses.empty());
    _defaultOutlineColor = _outlinePasses.front()->color();
}
    
void
App::updateLogic(Renderer& renderer, const XRService& xrService)
{
    auto handAnchors = xrService.handAnchors();
    onHandUpdate(renderer, handAnchors);
}
    
void
App::onHandUpdate(Renderer& renderer, const XRHandAnchors& anchors)
{
    _interactionManager.process(anchors);

    updateSelection(renderer, anchors);
}

void
App::onInteractionStateChanged(const XRInteraction::Ptr& interaction)
{
    const auto state = interaction->state();
    if (state == XRInteraction::State::inactive)
    {
        if ((interaction == _dragInteraction) || (interaction == _scaleInteraction))
        {
            _world->setSelection({});
        }
    }
    else if ((state == XRInteraction::State::active) || (state == XRInteraction::State::possible))
    {
        if (interaction == _dragInteraction)
        {
            const auto* payload = _dragInteraction->statePayload();
            ASSERT(payload != nullptr);
            _world->setSelection(payload->entry.object);
            
            setOutlineColor({ 0.f, 1.f, 0.f, 1.f });
        }
        else if (interaction == _scaleInteraction)
        {
            const auto* payload = _scaleInteraction->activePayload();
            ASSERT(payload != nullptr);
            _world->setSelection(payload->entry.object);
            
            setOutlineColor({ 1.f, 0.f, 0.f, 1.f });
        }
    }
}

void
App::setOutlineColor(float4 c)
{
    for (auto pass: _outlinePasses)
    {
        pass->setColor(c);
    }
}

void
App::updateSelection(Renderer& renderer, const XRHandAnchors& anchors)
{
    for (const auto& interaction: _interactionManager.interactions())
    {
        const auto state = interaction->state();
        if (state != XRInteraction::State::inactive)
        {
            return;
        }
    }
    
    // Select the object that is close enough
    XRHandAnchorsWithDistance anchorsWithDist { anchors };
    findClosestObject(_world->rootObject(), anchorsWithDist);
    const auto closestChiralityOpt = anchorsWithDist.closestAnchorChirality();
    
    if (!closestChiralityOpt.has_value())
    {
        return;
    }
    
    if (anchorsWithDist.distance(closestChiralityOpt.value()).distance <= 0.05f)
    {
        const auto closestChirality = closestChiralityOpt.value();
        auto object = anchorsWithDist.distance(closestChiralityOpt.value()).object;
        
        _world->setSelection(object);
        setOutlineColor(_defaultOutlineColor);
    }
    else
    {
        _world->setSelection({});
    }
}
        
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
        
        _app->prepare(*_renderer);
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
