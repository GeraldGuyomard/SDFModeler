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
        _renderer->invalidateCameraTransforms();
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
    
    _mtkView.depthStencilPixelFormat = _configuration->depthPixelFormat;
    _mtkView.colorPixelFormat = _configuration->colorPixelFormat;
    _mtkView.sampleCount = _configuration->sampleCount;
    
    return true;
}

RenderTargetConfiguration::CPtr
MTKViewRendererDelegate::presentConfiguration() const
{
    return _configuration;
}

id<MTLDevice> _Nonnull 
MTKViewRendererDelegate::getMTLDevice() const
{
    return _mtkView.device;
}

size_t
MTKViewRendererDelegate::cameraInfoCount() const
{
    return 1;
}

CameraInfo
MTKViewRendererDelegate::cameraInfo(size_t index, const Camera::Ptr& camera) const
{
    ASSERT(index == 0);
    
    CameraInfo info;
    
    CAMetalLayer* layer = (CAMetalLayer*) _mtkView.layer;
    const CGSize size = layer.drawableSize;
    info.setViewportSize(float2 { float(size.width), float(size.height) });
    
    const CGSize s = _mtkView.bounds.size;
    info.setViewportSizeInPoints({ float(s.width), float(s.height) });
    
    info.setProjectionMatrix(camera->computeProjectionMatrix(info.viewportSizeInPoints()));
    
    return info;
}

MTLRenderPassDescriptor* _Nullable
MTKViewRendererDelegate::currentRenderPassDescriptor() const
{
    return _mtkView.currentRenderPassDescriptor;
}

void
MTKViewRendererDelegate::presentDrawable(id<MTLCommandBuffer> _Nonnull commandBuffer)
{
    [commandBuffer presentDrawable:_mtkView.currentDrawable];
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
