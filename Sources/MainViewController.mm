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
#include "Composition3D.h"

// some static initializers
TObject3DFactoryRegistration s_SphereRegistration {"Sphere", SDFSphere { 0.5f } };
TObject3DFactoryRegistration s_BoxRegistration {"Box", SDFBox { float3 {0.5f, 0.5f, 0.5} } };
TObject3DFactoryRegistration s_RoundedBoxRegistration {"Rounded Box", SDFRoundedBox { float3 {0.5f, 0.5f, 0.5}, 0.1f } };

@interface MainViewController()
@end

@implementation MainViewController
{
    MTKView* _view;
    std::unique_ptr<Renderer> _renderer;
    
    World _world;
    
    Interaction::Ptr _interaction;
}

static __weak MainViewController* s_Instance = nil;

+(MainViewController*)instance
{
    return s_Instance;
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
    }
}

- (World&) world
{
    return _world;
}

- (Renderer*)renderer
{
    return _renderer.get();
}

- (void)loadWorld
{
    auto white = _world.addMaterial(float4 { 1, 1, 1, 1 });
    auto whiteSphere = std::make_shared<TObject3D<Sphere>>(Sphere { { 0.6f }, { float3 { 0, 1, -0.1f } } });
    whiteSphere->setMaterial(white);
    
    _world.addObject(whiteSphere);
    
    constexpr float kZ = 0;
    
    auto red = _world.addMaterial(float4 { 1, 0, 0, 1 });
    auto redSphere = std::make_shared<TObject3D<Sphere>>(Sphere { { 0.5f }, { float3 { -1, 0, kZ } } });
    redSphere->setMaterial(red);
    
    _world.addObject(redSphere);
    
    float3 pos = float3 {0.5, 0, kZ};
    
    constexpr float s = 0.5f;
    
    auto yellow = _world.addMaterial(float4 { 1, 1, 0, 1 });
    auto roundedYellowBox = std::make_shared<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.4f, 0.6f, 0.4f }, 0.1f }, // geometry
        { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s }
    });
    roundedYellowBox->setMaterial(yellow);
    _world.addObject(roundedYellowBox);
    
    auto whiteBoxHalf = std::make_shared<TObject3D<Box>>(Box
    {
        { float3 { 0.4f * s, 0.6f * s, 0.4f * s } }, // geometry
        { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) }
    });
    whiteBoxHalf->setMaterial(white);
    _world.addObject(whiteBoxHalf);
    
    auto blue = _world.addMaterial(float4 { 0, 0, 1, 1 });
    
    auto blueSphere = std::make_shared<TObject3D<Sphere>>(Sphere
    {
        { 0.4f },
        { pos }
    });
    
    blueSphere->setMaterial(blue);
    _world.addObject(blueSphere);
    
    auto green = _world.addMaterial(float4 { 0, 1, 0, 1 });
    
    auto greenSphere = std::make_shared<TObject3D<Sphere>>(Sphere
    {
        { 0.45f },
        { float3 { -1, 1, kZ } }
    });
    greenSphere->setMaterial(green);
    _world.addObject(greenSphere);
    
    const float3 compositionOrigin { -2., 0.f, kZ + 0.5f };
    
    auto spherePart = std::make_shared<TObject3D<Sphere>>(Sphere
    {
        { 0.4f }, // geom
        { float3 { 0, 0.6f, 0 } }
    });
    
    auto boxPart = std::make_shared<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.2, 0.4, 0.2 }, 0.1 }, // geometry
        { float3 { 0, 0, 0 } } // transform
    });
    
    auto negativeSpherePart = std::make_shared<TObject3D<Sphere>>(Sphere
    {
        { 0.4f }, // geom
        { float3 { 0, 0.25, 0.5f } } // transform
    });
    negativeSpherePart->setOperation(Object3D::Operation::substraction);
    
    auto negativeRoundedBoxPart = std::make_shared<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.1f, 0.1f, 0.3f }, 0.05f }, // geom
        { float3 { 0, -0.5f, 0.5f }, float3 {0, 0, 1}, degToRad(45.f) } // transform
    });
    negativeRoundedBoxPart->setOperation(Object3D::Operation::substraction);
    
    auto sdfUnionMaterial = _world.addMaterial(float4 { 0, 1, 1, 1 });
    auto sdfUnion = std::make_shared<Composition3D>();
    sdfUnion->setMaterial(sdfUnionMaterial);
    sdfUnion->setLocalTransform(matrix4x4_translation(compositionOrigin));
    
    sdfUnion->addChild(spherePart);
    sdfUnion->addChild(boxPart);
    sdfUnion->addChild(negativeSpherePart);
    sdfUnion->addChild(negativeRoundedBoxPart);
    
    _world.addObject(sdfUnion);
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    s_Instance = self;
    
    [self loadWorld];
    
    _view = (MTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[View alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = std::make_unique<Renderer>(_view);
}

- (IBAction)undo:(id)source
{
    self.world.commandHistory().undo();
}

- (IBAction)redo:(id)source
{
    self.world.commandHistory().redo();
}

@end
