//
//  GameViewController.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "GameViewController.h"
#import "Renderer.h"
#include "CommonDefinitions.h"

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
    const bool shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    
    auto pt = event.locationInWindow;
    simd_float2 delta { float(pt.x - _initialPos.x), float(pt.y - _initialPos.y) };
    
    simd_float3 right, up, fwd, pos;
    decompose(_initialCameraTransform, right, up, fwd, pos);
    
    if (shift)
    {
        constexpr float k = -1.f / 1000.f;
        
        delta *= k;
        
        pos += right * delta.x;
        pos += up * delta.y;
        
        auto newTransform = recompose(right, up, fwd, pos);
        _renderer.cameraTransform = newTransform;
    }
    else
    {
        // Orbit
        
        // yaw
        const auto yaw = matrix4x4_rotation(-delta.x * 1e-3f, simd_float3 { 0, 1, 0 }, _orbitOrigin);
        
        auto newPos = simd_mul(yaw, simd_float4 { pos.x, pos.y, pos.z, 1.f });
        pos = newPos.xyz;
        
        fwd = simd_normalize(pos - _orbitOrigin);
        right = simd_cross(up, fwd);
        
        auto newTransform = recompose(right, up, fwd, pos);
        
        // pitch
        decompose(newTransform, right, up, fwd, pos);
        
        const auto pitch = matrix4x4_rotation(delta.y * 1e-3f, simd_float3 { 1, 0, 0 }, _orbitOrigin);
        
        newPos = simd_mul(pitch, simd_float4 { pos.x, pos.y, pos.z, 1.f });
        pos = newPos.xyz;
        
        fwd = simd_normalize(pos - _orbitOrigin);
        up = simd_mul(yaw, simd_float4 { up.x, up.y, up.z, 0.f }).xyz;
        
        newTransform = recompose(right, up, fwd, pos);
        
        _renderer.cameraTransform = newTransform;
    }
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
