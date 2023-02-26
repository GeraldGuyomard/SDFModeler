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

- (void)mouseDragged:(NSEvent *)event
{
    float dX = event.deltaX;
    float dY = event.deltaY;
    
    auto transform = _renderer.cameraTransform;
    
    simd_float3 pos = translation(transform);
    
    constexpr float k = 1.f / 1000.f;
    
    pos.x += dX * k;
    pos.y += -dY * k;
    
    setTranslation(transform, pos);
    
    _renderer.cameraTransform = transform;
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto transform = _renderer.cameraTransform;
    auto pos = translation(transform);
    
    pos.z += d;
    setTranslation(transform, pos);
    
    _renderer.cameraTransform = transform;
}

@end
