//
//  MainViewController+macOS.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerMacOS.h"
#include "World.h"

@implementation MainViewControllerMacOS
{
    bool _shift;
    CGPoint _initialPos;
    simd_float4x4 _initialCameraTransform;
    simd_float3 _orbitOrigin;
}

- (void)rightMouseDown:(NSEvent *)event
{
    NSPoint ptInPixels = [self.view convertPoint:event.locationInWindow fromView:nil];
    
    NSRect r = self.view.frame;
    float2 size { float(r.size.width), float(r.size.height) };
    float2 p { float(ptInPixels.x), float(ptInPixels.y) };
    
    p = pixelToNDC(size, p);
    
    auto renderer = self.renderer;
    
    const auto& uniforms = renderer->uniforms();
    const auto& serializedWorld = renderer->serializedWorld();
    
    const auto pixel = renderDefault(p, uniforms, serializedWorld);
    
    NSLog(@"pixel R=%2.2f G=%2.2f B=%2.2f A=%2.2f\n", pixel.r, pixel.g, pixel.b, pixel.a);
}

- (void)mouseDown:(NSEvent *)event
{
    _shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    
    _initialPos = event.locationInWindow;
    _initialCameraTransform = self.renderer->camera()->worldTransform();
    
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
        self.renderer->camera()->setWorldTransform(newTransform);
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
        
        self.renderer->camera()->setWorldTransform(newTransform);
    }
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto camera = self.renderer->camera();
    
    auto transform = camera->worldTransform();
    auto pos = translation(transform);
    
    pos += d * forward(transform);
    setTranslation(transform, pos);
    
    camera->setWorldTransform(transform);
}

@end
