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

#include "World.h"
#include "Composition3D.h"

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

-(Interaction*) interaction
{
    return _interaction.get();
}

-(void)setInteraction:(Interaction::Ptr)interaction
{
    _interaction = std::move(interaction);
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
    
    auto spherePart = std::make_shared<TObject3D<Sphere>>(Sphere
    {
        { 0.4f }, // geom
        { float3 { -2., 0.6, kZ + 0.5f } }
    });
    
    auto boxPart = std::make_shared<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.2, 0.4, 0.2 }, 0.1 }, // geometry
        { float3 { -2., 0, kZ + 0.5f } } // transform
    });
    
    auto negativeSpherePart = std::make_shared<TObject3D<Sphere>>(Sphere
    {
        { 0.4f }, // geom
        { float3 { -2., 0.25, kZ + 1.f } } // transform
    });
    
    auto negativeRoundedBoxPart = std::make_shared<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.1f, 0.1f, 0.3f }, 0.05f }, // geom
        { float3 { -2., -0.5, kZ + 1.f }, float3 {0, 0, 1}, degToRad(45.f) } // transform
    });
    
    auto sdfUnionMaterial = _world.addMaterial(float4 { 0, 1, 1, 1 });
    auto sdfUnion = std::make_shared<Composition3D>();
    sdfUnion->setMaterial(sdfUnionMaterial);
    
    sdfUnion->addAdditiveObject(spherePart);
    sdfUnion->addAdditiveObject(boxPart);
    sdfUnion->addSubstractiveObject(negativeSpherePart);
    sdfUnion->addSubstractiveObject(negativeRoundedBoxPart);
    
    _world.addObject(std::move(sdfUnion));
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

@end
