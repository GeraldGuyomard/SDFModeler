//
//  GameViewController.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "GameViewController.h"
#import "Renderer.h"
#include "CommonDefinitions.h"

#include "World.h"

@implementation GameViewController
{
    MTKView* _view;
    Renderer* _renderer;
    
    bool _shift;
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

- (void)rightMouseDown:(NSEvent *)event
{
    NSPoint ptInPixels = [self.view convertPoint:event.locationInWindow fromView:nil];
    
    NSRect r = self.view.frame;
    float2 size { float(r.size.width), float(r.size.height) };
    
    float2 p { float(ptInPixels.x), float(ptInPixels.y) };
    
    // NDC [-1, +1]
    p.x = 2.f * (p.x - (0.5f * size.x)) / size.x;
    p.y = 2.f * (p.y - (0.5f * size.y)) / size.y;
    
    const auto* uniforms = _renderer.uniforms;
    const auto pixel = render(p, *uniforms);
    
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

@end
