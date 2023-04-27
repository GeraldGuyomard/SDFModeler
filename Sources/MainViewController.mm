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

// some static initializers
TObject3DFactoryRegistration s_SphereRegistration {"Sphere", SDFSphere { 0.5f } };
TObject3DFactoryRegistration s_BoxRegistration {"Box", SDFBox { float3 {0.5f, 0.5f, 0.5} } };
TObject3DFactoryRegistration s_RoundedBoxRegistration {"Rounded Box", SDFRoundedBox { float3 {0.5f, 0.5f, 0.5}, 0.1f } };
TObject3DFactoryRegistration s_TorusRegistration {"Torus", SDFTorus { 0.5f, 0.25f } };
TObject3DFactoryRegistration s_CylinderRegistration {"Cylinder", SDFCylinder { 0.3f, 0.7f } };


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

@implementation MainViewController
{
    MTKView* _view;
    CGFloat _nativeContentScale;
    std::unique_ptr<Renderer> _renderer;
    
    WorldPtr _world;
    
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

- (void)loadWorld
{
    _world = World::make();
    auto rootObject = _world->rootObject();
    
    constexpr float kZ = 0;
    
    auto white = _world->addMaterial(float4 { 1, 1, 1, 1 });
    auto whiteSphere = std::make_shared<TObject3D<SDFSphere>>(_world, SDFSphere { 0.6f });
    whiteSphere->setLocalTransform(RSTTransformer { float3 { 0, 1, -0.1f } } );
    whiteSphere->setMaterial(white);
    
    rootObject->addChild(whiteSphere);
    
    auto red = _world->addMaterial(float4 { 1, 0, 0, 1 });
    auto redSphere = std::make_shared<TObject3D<SDFSphere>>(_world, SDFSphere { 0.5f });
    redSphere->setLocalTransform(RSTTransformer { float3 { -1, 0, kZ } } );
    redSphere->setMaterial(red);
    
    rootObject->addChild(redSphere);
    
    float3 pos = float3 {0.5, 0, kZ};
    
    constexpr float s = 0.5f;
    
    auto yellow = _world->addMaterial(float4 { 1, 1, 0, 1 });
    auto roundedYellowBox = std::make_shared<TObject3D<SDFRoundedBox>>(_world,
                                                                       SDFRoundedBox { float3 { 0.4f, 0.6f, 0.4f }, 0.1f });
    roundedYellowBox->setLocalTransform(RSTTransformer { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s });
    roundedYellowBox->setMaterial(yellow);
    rootObject->addChild(roundedYellowBox);
    
    auto whiteBoxHalf = std::make_shared<TObject3D<SDFBox>>(_world,
                                                            SDFBox { float3 { 0.4f * s, 0.6f * s, 0.4f * s } });
    whiteBoxHalf->setLocalTransform(RSTTransformer { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) });
    whiteBoxHalf->setMaterial(white);
    rootObject->addChild(whiteBoxHalf);
    
    auto blue = _world->addMaterial(float4 { 0, 0, 1, 1 });
    
    auto blueSphere = std::make_shared<TObject3D<SDFSphere>>(_world, SDFSphere { 0.4f });
    blueSphere->setLocalTransform(RSTTransformer { pos });
    blueSphere->setMaterial(blue);
    rootObject->addChild(blueSphere);
    
    auto green = _world->addMaterial(float4 { 0, 1, 0, 1 });
    
    auto greenSphere = std::make_shared<TObject3D<SDFSphere>>(_world, SDFSphere { 0.45f });
    greenSphere->setLocalTransform(RSTTransformer { float3 { -1, 1, kZ } });
    greenSphere->setMaterial(green);
    rootObject->addChild(greenSphere);
    
    const float3 compositionOrigin { -2., 0.f, kZ + 0.5f };
    
    auto spherePart = std::make_shared<TObject3D<SDFSphere>>(_world, SDFSphere { 0.4f });
    spherePart->setLocalTransform(RSTTransformer { float3 { 0, 0.6f, 0 } });
    
    auto boxPart = std::make_shared<TObject3D<SDFRoundedBox>>(_world, SDFRoundedBox
        { float3 { 0.2, 0.4, 0.2 }, 0.1 }
    );
    boxPart->setLocalTransform(RSTTransformer { float3 { 0, 0, 0 } });
    
    auto negativeSpherePart = std::make_shared<TObject3D<SDFSphere>>(_world, SDFSphere { 0.4f });
    negativeSpherePart->setLocalTransform(RSTTransformer { float3 { 0, 0.25, 0.5f } });
    negativeSpherePart->setOperation(SDFOperation::substraction);
    
    auto negativeRoundedBoxPart = std::make_shared<TObject3D<SDFRoundedBox>>(_world, SDFRoundedBox
    { float3 { 0.1f, 0.1f, 0.3f }, 0.05f });
    negativeRoundedBoxPart->setLocalTransform(RSTTransformer { float3 { 0, -0.5f, 0.5f }, float3 {0, 0, 1}, degToRad(45.f) });
    negativeRoundedBoxPart->setOperation(SDFOperation::substraction);
    
    auto sdfUnionMaterial = _world->addMaterial(float4 { 0, 1, 1, 1 });
    auto sdfUnion = std::make_shared<Object3D>(_world);
    sdfUnion->setMaterial(sdfUnionMaterial);
    sdfUnion->setLocalTransform(matrix4x4_translation(compositionOrigin));
    sdfUnion->setShouldChildrenShareId(true);
    
    sdfUnion->addChild(spherePart);
    sdfUnion->addChild(boxPart);
    sdfUnion->addChild(negativeSpherePart);
    sdfUnion->addChild(negativeRoundedBoxPart);
    
    rootObject->addChild(sdfUnion);
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self loadWorld];
    
    _view = (MTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();
    _nativeContentScale =  self.view.layer.contentsScale;
    
    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[View alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = std::make_unique<Renderer>(_view);
    
    _renderer->setWorld(self.world);
    auto camera = std::make_shared<Camera>(_world);
    _renderer->setCamera(camera);
    
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
    
    auto root = self.world->rootObject();
    const auto transform = camera->computeFrameTransform(root);
    camera->setWorldTransform(transform);
    
    self.world->setInvalidationCallback([wSelf](const auto& world)
    {
        if (auto self = wSelf)
        {
            if (auto renderer = self.renderer)
            {
                renderer->invalidate();
            }
        }
    });
}

- (IBAction)undo:(id)source
{
    self.world->commandHistory().undo();
}

- (IBAction)redo:(id)source
{
    self.world->commandHistory().redo();
}

- (void)frameAtPosition:(float2)pos
{
    // zoom in/out
    const auto result = self.renderer->pick(pos);
    
    auto object = self.world->rootObject()->objectByID(result.objectID);
    
    if (object == nullptr)
    {
        object = self.world->rootObject();
    }
    
    auto camera = self.renderer->camera();
    camera->setLookAtPositionProvider(object);
    
    const auto cameraPos = camera->computeFramePosition(object);
    
    auto animation = std::make_shared<MoveCameraAnimation>(camera, 0.25f, cameraPos);
    [self setCameraAnimation:animation];
}

- (void)addAnimation:(Animation::Ptr)animation
{
    _animationEntries.push_back({animation});
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

@end
