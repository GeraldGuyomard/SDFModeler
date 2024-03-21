//
//  MainViewController.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "MainViewController.h"
#include "CommonDefinitions.h"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "RenderFunctions.h"
#include <chrono>

#include "MoveCameraAnimation.h"

#include "MTKViewRendererDelegate.h"

#include "Commands/RemoveObjectCommand.h"
#include "Commands/GroupSelectionCommand.h"
#include "Commands/ToggleObjectOperationCommand.h"

#include "CameraRig.h"
#include "WorldHelpers.h"

#include "SDFRenderPass.h"

@interface MainViewController()
@end

struct AnimationEntry final
{
    bool started = false;
    Animation::Ptr animation;
    
    AnimationEntry() = default;
    
    AnimationEntry(const Animation::Ptr& anim)
    : animation(anim)
    {}
};

class Delegate final : public WorldDelegate
{
public:
    
    Delegate(MainViewController* controller)
    : _controller(controller)
    {}
    
    void onChange(const WorldPtr&) override
    {
        if (auto self = _controller)
        {
            if (auto renderer = _controller.renderer)
            {
                renderer->invalidate();
            }
        }
    }
    
    void onSelectionChanged(const WorldPtr& world, const Object3DSelection& oldObject, const Object3DSelection& newObject) override
    {
        [_controller onSelectionChange];
    }
    
private:
    __weak MainViewController* _controller;
};

@implementation MainViewController
{
    MTKView* _view;
    CGFloat _nativeContentScale;
    std::unique_ptr<Renderer> _renderer;
    
    WorldPtr _world;
    std::shared_ptr<Delegate> _delegate;
    
    Interaction::Ptr _interaction;
    
    HighResClock::time_point _baseTime;
    std::vector<AnimationEntry> _animationEntries;
    
    Animation::Ptr _cameraAnimation;
}

-(Interaction::Ptr) interaction
{
    return _interaction;
}

-(void)setInteraction:(Interaction::Ptr)interaction
{
    if (_interaction != interaction)
    {
        if (_interaction != nullptr)
        {
            _interaction->commit();
        }
        
        _interaction = interaction;
        
        [self setCameraAnimation:nullptr];
    }
}

- (WorldPtr) world
{
    return _world;
}

- (Renderer*)renderer
{
    return _renderer.get();
}

- (CGFloat)nativeContentScale
{
    return _nativeContentScale;
}

void visitTypes(const Object3D::Ptr& object)
{
    auto type = object->geometryType();
    if (type == &TType<SDFSphere>::instance())
    {
        const TPropertyValue value = type->getPropertyValue(object.get(), "radius");
        if (auto v = std::get_if<float>(&value))
        {
            int a;
            a = 1;
        }
        
    }
    
    for (auto child : object->children())
    {
        visitTypes(child);
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _world = makeDefaultWorld();
    
    {
        // Testing types
        visitTypes(_world->rootObject());
    }
    
    _view = (MTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();
    _nativeContentScale =  self.view.layer.contentsScale;
    
    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[View alloc] initWithFrame:self.view.frame];
        return;
    }

    auto delegate = std::make_unique<MTKViewRendererDelegate>(_view);
    _renderer = std::make_unique<Renderer>(self.world, std::move(delegate));
    
    _baseTime = HighResClock::now();
    
    __weak auto wSelf = self;
    self.renderer->setRenderCallback([wSelf](auto& renderer)
    {
        if (auto self = wSelf)
        {
            auto now = HighResClock::now();
            const auto dT = now - _baseTime;
            
            const float t = std::chrono::duration_cast<std::chrono::milliseconds>(dT).count() / 1000.f;
            
            [self update:t];
        }
    });
    
    _delegate = std::make_shared<Delegate>(self);
    self.world->setDelegate(_delegate);
    
    _renderer->invalidate();
}

- (void)reframeAllImmediately
{
    auto root = self.world->rootObject();
    
    auto camera = _renderer->cameraRig();

    const auto transform = camera->computeFrameTransform(root);
    camera->setWorldTransform(transform);
}

- (IBAction)undo:(id)source
{
    self.world->commandHistory().undo();
}

- (IBAction)redo:(id)source
{
    self.world->commandHistory().redo();
}

- (IBAction)delete:(id)source
{
    auto cmd = std::make_shared<RemoveObjectCommand>(self.world->selection());
    self.world->commandHistory().run(cmd);
}

- (IBAction)group:(id)source
{
    auto cmd = std::make_shared<GroupSelectionCommand>(self.world->selection());
    self.world->commandHistory().run(cmd);
}

- (IBAction)toggleOperation:(id)source
{
    auto cmd = std::make_shared<ToggleObjectOperationCommand>(self.world->selection());
    self.world->commandHistory().run(cmd);
}

- (void)setRenderStyle:(RenderStyle)style
{
    auto renderer = self.renderer;
    auto pass = renderer->sdfRenderPass();
    pass->setRenderStyle(style);
    
    renderer->invalidate();
}

- (IBAction)selectPhongRendering:(id)source
{
    [self setRenderStyle:RenderStyle::phong];
}

- (IBAction)selectCellShadedRendering:(id)source
{
    [self setRenderStyle:RenderStyle::cellShaded];
}

- (IBAction)selectFlatRendering:(id)source
{
    [self setRenderStyle:RenderStyle::flat];
}

- (void)frameAtPosition:(float2)pos owner:(BOOL)frameOwner
{
    // zoom in/out
    const auto result = self.renderer->pick(pos);
    
    auto object = self.world->rootObject()->objectByID(result.objectID);
    
    if (object == nullptr)
    {
        object = self.world->rootObject();
    }
    else if (frameOwner)
    {
        object = object->owner();
    }
    
    auto cameraRig = self.renderer->cameraRig();
    cameraRig->setLookAtPositionProvider(object);
    
    const auto cameraPos = cameraRig->computeFramePosition(object);
    
    auto animation = std::make_shared<MoveCameraAnimation>(cameraRig, 0.25f, cameraPos);
    [self setCameraAnimation:animation];
}

- (void)addAnimation:(Animation::Ptr)animation
{
    _animationEntries.push_back({animation});
    _renderer->invalidate();
}

- (void)removeAnimation:(Animation::Ptr)animation
{
    for (auto it = _animationEntries.begin(); it != _animationEntries.end(); ++it)
    {
        if ((*it).animation == animation)
        {
            _animationEntries.erase(it);
            break;
        }
    }
}

- (void)update:(float)t
{
    for (auto it = _animationEntries.begin(); it != _animationEntries.end();)
    {
        auto& entry = *it;
        if (entry.animation->isFinished())
        {
            it = _animationEntries.erase(it);
        }
        else
        {
            if (!entry.started)
            {
                entry.started = true;
                entry.animation->start(t);
            }
            else
            {
                entry.animation->update(t);
            }
            
            ++it;
        }
    }
}

- (Animation::Ptr) cameraAnimation
{
    return _cameraAnimation;
}

- (void)setCameraAnimation:(Animation::Ptr)animation
{
    if (animation != _cameraAnimation)
    {
        if (_cameraAnimation != nullptr)
        {
            [self removeAnimation:_cameraAnimation];
        }
        
        _cameraAnimation = animation;
        
        if (_cameraAnimation != nullptr)
        {
            [self addAnimation:_cameraAnimation];
        }
    }
}

- (void) onSelectionChange
{
    
}

@end
