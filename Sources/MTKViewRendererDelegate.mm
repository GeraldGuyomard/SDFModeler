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

#define CONTINUOUS_RENDER 1

- (void)drawInMTKView:(nonnull MTKView *)view
{
    if (_renderer != nullptr)
    {
        _renderer->render();
        
#if CONTINUOUS_RENDER
        _renderer->invalidate();
#endif
    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    if (_renderer != nullptr)
    {
        _renderer->delegate()->updateViewportSize();
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
    
    _mtkView.depthStencilPixelFormat = _configuration->depthPixelFormat;
    _mtkView.colorPixelFormat = _configuration->colorPixelFormat;
    _mtkView.sampleCount = _configuration->sampleCount;
    
    _cameraRig = CameraRig::make(renderer->world(), 1);
    
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

CameraRig::Ptr
MTKViewRendererDelegate::cameraRig() const
{
    return _cameraRig;
}

void
MTKViewRendererDelegate::updateViewportSize()
{
    CGSize drawableSize = _mtkView.bounds.size;
    const float s = _mtkView.layer.contentsScale;
    drawableSize.width *= s;
    drawableSize.height *= s;
    
    ASSERT(drawableSize.width > 0.f);
    ASSERT(drawableSize.height > 0.f);
    
    const float2 viewportSize { float(drawableSize.width), float(drawableSize.height) };

    if (!_lastUpdatedViewportSize.has_value() || (_lastUpdatedViewportSize.value().x != viewportSize.x) || (_lastUpdatedViewportSize.value().y != viewportSize.y))
    {
        _lastUpdatedViewportSize = viewportSize;
        
        for (const auto& camera : _cameraRig->cameras())
        {
            camera->setViewportSize(viewportSize);
            
#if 1
            camera->setInverseZ(true);
            
            auto intrinsics = std::make_unique<TangentsCameraIntrinsics>();
            
            const float ratio = viewportSize.y / viewportSize.x;
            constexpr float tangentX = 1.f;
            const float tangentY = tangentX * ratio;
            
            intrinsics->setTangents(float4 { tangentX, tangentX, tangentY, tangentY });
            
            camera->setIntrinsics(std::move(intrinsics));
#else
            camera->setInverseZ(false);
            
            auto intrinsics = std::make_unique<FOVCameraIntrinsics>();
            
            camera->setIntrinsics(std::move(intrinsics));
#endif
            
            float nearZNDC = 0.f;
            float farZNDC = 0.5f;
            
            if (camera->inverseZ())
            {
                std::swap(nearZNDC, farZNDC);
            }
            
            camera->setNearZInNDC(nearZNDC);
            camera->setFarZInNDC(farZNDC);
            
            const auto projMatrix = camera->intrinsics()->computeProjectionMatrix(viewportSize, camera->inverseZ());
            
            camera->setProjectionMatrix(projMatrix);
        }
    }
}

MTLRenderPassDescriptor* _Nullable
MTKViewRendererDelegate::renderPassDescriptor(size_t cameraIndex) const
{
    ASSERT(cameraIndex == 0);
    return _mtkView.currentRenderPassDescriptor;
}

MTLCompareFunction
MTKViewRendererDelegate::depthCompareFunction() const
{
    return MTLCompareFunctionLessEqual;
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
