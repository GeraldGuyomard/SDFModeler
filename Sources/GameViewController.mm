//
//  GameViewController.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "GameViewController.h"
#import "Renderer.h"

@implementation GameViewController
{
    MTKView* _view;
    Renderer* _renderer;
    
    CGPoint _initialPos;
    simd_float4x4 _initialCameraTransform;
    simd_float3 _orbitOrigin;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = [[Renderer alloc] initWithMetalKitView:_view];

    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];

    _view.delegate = _renderer;
}

- (void)mouseDown:(NSEvent *)event
{
    _initialPos = event.locationInWindow;
    _initialCameraTransform = _renderer.cameraTransform;
    
    const auto position = _initialCameraTransform.columns[3].xyz;
    const auto direction = simd_normalize(_initialCameraTransform.columns[2].xyz);
    
    _orbitOrigin = position - (direction * 5.f);
}

- (void)mouseDragged:(NSEvent *)event
{
#if 1
    auto pt = event.locationInWindow;
    simd_float2 delta { float(pt.x - _initialPos.x), float(pt.y - _initialPos.y) };
    
    // Orbit
    const auto yaw = matrix4x4_rotation(delta.x * 1e-3f, simd_float3 { 0, 1, 0 }, _orbitOrigin);
    
    auto cam = _initialCameraTransform;
    auto pos = cam.columns[3].xyz;
    
    const auto newPos = simd_mul(yaw, simd_float4 { pos.x, pos.y, pos.z, 1.f });
    pos = newPos.xyz;
    
    const auto fwd = simd_normalize(pos - _orbitOrigin);
    const auto up = cam.columns[1].xyz;
    const auto right = simd_cross(up, fwd);
    
    cam.columns[0].xyz = right;
    cam.columns[1].xyz = up;
    cam.columns[2].xyz = fwd;
    cam.columns[3].xyz = pos;
    
    _renderer.cameraTransform = cam;
    
#else
    float dX = event.deltaX;
    float dY = event.deltaY;
    
    auto transform = _renderer.cameraTransform;
    
    simd_float3 pos = translation(transform);
    
    constexpr float k = 1.f / 1000.f;
    
    pos.x += dX * k;
    pos.y += -dY * k;
    
    setTranslation(transform, pos);
    
    _renderer.cameraTransform = transform;
#endif
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto transform = _renderer.cameraTransform;
    auto fwd = simd_normalize(transform.columns[2].xyz);
    
    auto pos = translation(transform);
    
    pos += d * fwd;
    setTranslation(transform, pos);
    
    _renderer.cameraTransform = transform;
}

@end
