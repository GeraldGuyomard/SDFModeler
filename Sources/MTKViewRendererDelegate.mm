//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "MTKViewRendererDelegate.h"

@interface MTKViewBridge : NSObject<MTKViewDelegate>

- (instancetype) initWithRenderer:(Renderer*)renderer;
- (void)terminate;

- (void)delayPause;

@end

@implementation MTKViewBridge
{
    Renderer* _renderer;
    NSTimer* _timer;
}

- (instancetype) initWithRenderer:(Renderer*)renderer
{
    if (self = [self init])
    {
        _renderer = renderer;
    }
    
    return self;
}

- (void)terminate
{
    _renderer = nullptr;
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    if (_renderer != nullptr)
    {
        _renderer->render();
    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    if (_renderer != nullptr)
    {
        _renderer->updateCameraTransforms();
        _renderer->invalidate();
    }
}

- (void)delayPause
{
    [_timer invalidate];
    _timer = [NSTimer scheduledTimerWithTimeInterval:0.1f target:self selector:@selector(onPause) userInfo:nil repeats:NO];
}

- (void)onPause
{
    if (_renderer != nullptr)
    {
        _renderer->pause();
    }
}

@end

MTKViewRendererDelegate::MTKViewRendererDelegate(MTKView* _Nonnull mtkView)
: _mtkView(mtkView)
{
}

MTKViewRendererDelegate::~MTKViewRendererDelegate()
{
    [_mtkViewBridge terminate];
}

bool
MTKViewRendererDelegate::init(Renderer* renderer)
{
    _mtkViewBridge = [[MTKViewBridge alloc] initWithRenderer:renderer];
    
    _mtkView.delegate = _mtkViewBridge;
    _mtkView.paused = YES;
    
    _mtkView.depthStencilPixelFormat = _configuration.depthStencilPixelFormat;
    _mtkView.colorPixelFormat = _configuration.colorPixelFormat;
    _mtkView.sampleCount = _configuration.sampleCount;
    
    return true;
}

RendererDelegateConfiguration
MTKViewRendererDelegate::configuration() const
{
    return _configuration;
}

id<MTLDevice> _Nonnull 
MTKViewRendererDelegate::getMTLDevice() const
{
    return _mtkView.device;
}

float2
MTKViewRendererDelegate::renderSize() const
{
    CAMetalLayer* layer = (CAMetalLayer*) _mtkView.layer;
    const CGSize size = layer.drawableSize;
    return float2 { float(size.width), float(size.height) };
}

float2
MTKViewRendererDelegate::renderSizeInPoints() const
{
    const CGSize s = _mtkView.bounds.size;
    return { float(s.width), float(s.height) };
}

MTLRenderPassDescriptor* _Nullable
MTKViewRendererDelegate::currentRenderPassDescriptor() const
{
    return _mtkView.currentRenderPassDescriptor;
}

id <MTLDrawable> _Nonnull
MTKViewRendererDelegate::currentDrawable() const
{
    return _mtkView.currentDrawable;
}

void
MTKViewRendererDelegate::invalidate()
{
    _mtkView.paused = NO;
    [_mtkViewBridge delayPause];
}

void
MTKViewRendererDelegate::pause()
{
    _mtkView.paused = YES;
}
