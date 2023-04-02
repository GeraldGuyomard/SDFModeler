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
    
    CameraController::Ptr _cameraController;
}

static __weak MainViewController* s_Instance = nil;

+(MainViewController*)instance
{
    return s_Instance;
}

-(CameraController*) cameraController
{
    return _cameraController.get();
}

-(void)setCameraController:(CameraController::Ptr)cameraController
{
    _cameraController = std::move(cameraController);
}

- (const World&) world
{
    return _world;
}

- (Renderer*)renderer
{
    return _renderer.get();
}

- (void)loadWorld
{
    auto& content = _world.content();
    
    auto whiteSphere = std::make_unique<TObject3D<Sphere>>(Sphere { { 0.6f }, { float3 { 0, 1, -0.1f } }, { float4 { 1, 1, 1, 1 } } });
    content.addObject(std::move(whiteSphere));
    
    constexpr float kZ = 0;
    
    auto redSphere = std::make_unique<TObject3D<Sphere>>(Sphere { { 0.5f }, { float3 { -1, 0, kZ } }, { float4 { 1, 0, 0, 1 } } });
    content.addObject(std::move(redSphere));
    
    float3 pos = float3 {0.5, 0, kZ};
    
    constexpr float s = 0.5f;
    
    auto roundedYellowBox = std::make_unique<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.4f, 0.6f, 0.4f }, 0.1f }, // geometry
        { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s }, // transform
        { float4 { 1, 1, 0, 1 } } // material
    });
    content.addObject(std::move(roundedYellowBox));
    
    auto whiteBoxHalf = std::make_unique<TObject3D<Box>>(Box
    {
        { float3 { 0.4f * s, 0.6f * s, 0.4f * s } }, // geometry
        { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) }, // transform
        { float4 { 1, 1, 1, 1 } } // material
    });
    content.addObject(std::move(whiteBoxHalf));
    
    auto blueSphere = std::make_unique<TObject3D<Sphere>>(Sphere
    {
        { 0.4f },
        { pos },
        { float4 { 0, 0, 1, 1 } }
    });
    content.addObject(std::move(blueSphere));
    
    auto greenSphere = std::make_unique<TObject3D<Sphere>>(Sphere
    {
        { 0.45f },
        { float3 { -1, 1, kZ } },
        { float4 { 0, 1, 0, 1 } }
    });
    content.addObject(std::move(greenSphere));
    
    auto spherePart = std::make_unique<TObject3D<Sphere>>(Sphere
    {
        { 0.4f }, // geom
        { float3 { -2., 0.6, kZ + 0.5f } }
    });
    
    auto boxPart = std::make_unique<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.2, 0.4, 0.2 }, 0.1 }, // geometry
        { float3 { -2., 0, kZ + 0.5f } } // transform
    });
    
    auto negativeSpherePart = std::make_unique<TObject3D<Sphere>>(Sphere
    {
        { 0.4f }, // geom
        { float3 { -2., 0.25, kZ + 1.f } } // transform
    });
    
    auto negativeRoundedBoxPart = std::make_unique<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.1f, 0.1f, 0.3f }, 0.05f }, // geom
        { float3 { -2., -0.5, kZ + 1.f }, float3 {0, 0, 1}, degToRad(45.f) } // transform
    });
    
    auto sdfUnion = std::make_unique<Composition3D>(
                                    RSTTransformer {} /* transform*/,
                                    ConstMaterial { float4 { 0, 1, 1, 1 } } /* material */);
    
    sdfUnion->addAdditiveObject(std::move(spherePart));
    sdfUnion->addAdditiveObject(std::move(boxPart));
    sdfUnion->addSubstractiveObject(std::move(negativeSpherePart));
    sdfUnion->addSubstractiveObject(std::move(negativeRoundedBoxPart));
    
    content.addObject(std::move(sdfUnion));
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
