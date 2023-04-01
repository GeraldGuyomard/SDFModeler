//
//  GameViewController.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "GameViewController.h"
#include "CommonDefinitions.h"

#include "World.h"
#include "Composition3D.h"

@implementation GameViewController
{
    MTKView* _view;
    Renderer* _renderer;
    
    World _world;
    
    bool _shift;
    CGPoint _initialPos;
    simd_float4x4 _initialCameraTransform;
    simd_float3 _orbitOrigin;
}

static GameViewController* s_Instance = nil;

+(GameViewController*)instance
{
    return s_Instance;
}

- (const World&) world
{
    return _world;
}

- (Renderer*)renderer
{
    return _renderer;
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
    
    constexpr float kGridGreyLevel = 0.5f;
    const float4 color{ kGridGreyLevel, kGridGreyLevel, kGridGreyLevel, 1 };
    //Plane grid({}, { float3(-10.f) }, { color } );
    
    Grid grid({}, { float3(-0.5f) }, { 0.1f , color });
    auto grid3d = std::make_unique<Grid3D>(grid);
    
    _world.environment().addObject(std::move(grid3d));
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

    _renderer = [[Renderer alloc] initWithMetalKitView:_view];

    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];

    _view.delegate = _renderer;
}

#if TARGET_OS_OSX

- (void)rightMouseDown:(NSEvent *)event
{
    NSPoint ptInPixels = [self.view convertPoint:event.locationInWindow fromView:nil];
    
    NSRect r = self.view.frame;
    float2 size { float(r.size.width), float(r.size.height) };
    float2 p { float(ptInPixels.x), float(ptInPixels.y) };
    
    p = pixelToNDC(size, p);
    
    const auto* uniforms = _renderer.uniforms;
    const auto* serializedWorld = _renderer.serializedWorld;
    
    const auto pixel = renderDefault(p, *uniforms, *serializedWorld);
    
    NSLog(@"pixel R=%2.2f G=%2.2f B=%2.2f A=%2.2f\n", pixel.r, pixel.g, pixel.b, pixel.a);
}

- (void)mouseDown:(NSEvent *)event
{
    _shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    
    _initialPos = event.locationInWindow;
    _initialCameraTransform = _renderer.cameraTransform;
    
    const auto position = _initialCameraTransform.columns[3].xyz;
    const auto direction = simd_normalize(_initialCameraTransform.columns[2].xyz);
    
    _orbitOrigin = position - (direction * 5.f);
}

- (void)mouseDragged:(NSEvent *)event
{
    auto pt = event.locationInWindow;
    simd_float2 delta { float(pt.x - _initialPos.x), float(pt.y - _initialPos.y) };
    
    auto decomp = decompose(_initialCameraTransform);
    
    if (_shift)
    {
        constexpr float k = -1.f / 1000.f;
        
        delta *= k;
        
        decomp.position += decomp.right * delta.x;
        decomp.position += decomp.up * delta.y;
        
        auto newTransform = recompose(decomp);
        _renderer.cameraTransform = newTransform;
    }
    else
    {
        // Orbit
        
        // yaw
        const auto yaw = matrix4x4_rotation(-delta.x * 1e-3f, float3 { 0, 1, 0 }, _orbitOrigin);
        
        auto newPos = yaw * make_float4(decomp.position, 1.f);
        decomp.position = newPos.xyz;
        
        decomp.forward = normalize(decomp.position - _orbitOrigin);
        decomp.right = cross(decomp.up, decomp.forward);
        
        auto newTransform = recompose(decomp);
        
        // pitch
        decomp = decompose(newTransform);
        
        const auto pitch = matrix4x4_rotation(delta.y * 1e-3f, float3 { 1, 0, 0 }, _orbitOrigin);
        
        newPos = pitch * make_float4(decomp.position, 1.f);
        decomp.position = newPos.xyz;
        
        decomp.forward = normalize(decomp.position - _orbitOrigin);
        decomp.up = (yaw * make_float4(decomp.up, 0.f)).xyz;
        
        newTransform = recompose(decomp);
        
        _renderer.cameraTransform = newTransform;
    }
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto transform = _renderer.cameraTransform;
    auto pos = translation(transform);
    
    pos += d * forward(transform);
    setTranslation(transform, pos);
    
    _renderer.cameraTransform = transform;
}

#else

// iOS
- (void) setContentScaleFactor:(CGFloat)sf size:(CGSize)size
{
    UIView* view = self.view;
    CAMetalLayer* metalLayer = (CAMetalLayer *)view.layer;
    
    metalLayer.contentsScale = sf;
    
    CGSize drawableSize;
    drawableSize.width = size.width * sf;
    drawableSize.height = size.height * sf;
    
    metalLayer.drawableSize = drawableSize;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    const CGSize size = self.view.bounds.size;
    [self setContentScaleFactor:1.f size:size];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    [self setContentScaleFactor:1.f size:size];
}

#endif // TARGET_OS_OSX

@end
