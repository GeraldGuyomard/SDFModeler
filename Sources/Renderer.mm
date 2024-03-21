//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>

#import "Renderer.h"

#import "ShaderTypes.h"

#include "SerializedWorldObject.h"
#include "FragmentShader/PhongShader.h"

#include "Object3D.h"
#include "RenderFunctions.h"

#include "SDFRenderPass.h"
#include "SelectionMattingRenderPass.h"
#include "SelectionOutlineRenderPass.h"
#include "WorkingPlaneRenderPass.h"

#include "MainViewController.h"

RendererDelegate::DepthInfo::DepthInfo(float clearDepth, MTLCompareFunction compareFunction, bool depthReadbackDownstream)
: clearDepth(clearDepth), compareFunction(compareFunction), depthReadbackDownstream(depthReadbackDownstream)
{}

float2
RendererDelegate::tileSize() const
{
    const float2 kDefaultTileSize { 64, 64 };
    //const float2 kDefaultTileSize { 128, 128 };
    //const float2 kDefaultTileSize { 256, 256 };
    //const float2 kDefaultTileSize { 1024, 1024 };
    //const float2 kDefaultTileSize { 2048, 2048 };
    //const float2 kDefaultTileSize { 4096, 4096 };
    
    return kDefaultTileSize;
}

Renderer::Renderer(const WorldPtr& world, RendererDelegate::Ptr delegate)
: _world(world), _delegate(std::move(delegate)),
_inFlightSemaphore(dispatch_semaphore_create(RenderPass::kMaxBuffersInFlight))
{
    _delegate->init(this);
    
    init();
}

Renderer::~Renderer()
{
    _delegate.reset();
}

void
Renderer::init()
{
    const auto device = _delegate->getMTLDevice();
    
    _commandQueue = [device newCommandQueue];
    _mtlLibrary = [device newDefaultLibrary];
    
    _sdfRenderPass = std::make_unique<SDFRenderPass>();
    _renderPasses.push_back(_sdfRenderPass.get());
    
    _selectionMattingRenderPass = std::make_unique<SelectionMattingRenderPass>();
    _renderPasses.push_back(_selectionMattingRenderPass.get());
    
    _selectionOutlineRenderPass = std::make_unique<SelectionOutlineRenderPass>();
    _renderPasses.push_back(_selectionOutlineRenderPass.get());
    
    _selectionOutlineRenderPass->setDepthTextureProvider([selectionMattingPass = _selectionMattingRenderPass.get()]()
    {
        return selectionMattingPass->targetDepthTexture();
    });
    
    _workingPlaneRenderPass = std::make_unique<WorkingPlaneRenderPass>();
    _renderPasses.push_back(_workingPlaneRenderPass.get());
    
    for (auto renderPass : _renderPasses)
    {
        renderPass->init(*this);
    }
}

void
Renderer::setRenderCallback(const RenderCallback& cb)
{
    _renderCallback = cb;
}

class FrameSubmission final
{
public:
    FrameSubmission(RendererDelegate* delegate)
    : _delegate(delegate)
    {
        if (!_delegate->startSubmission())
        {
            _delegate = nullptr;
        }
    }
    
    ~FrameSubmission()
    {
        end();
    }
    
    bool isValid() const
    {
        return _delegate != nullptr;
    }
    
    void end()
    {
        if (_delegate != nullptr)
        {
            _delegate->endSubmission();
            _delegate = nullptr;
        }
    }
    
private:
    RendererDelegate* _delegate;
};

void
Renderer::render()
{
    /// Per frame updates here
    auto now = HighResClock::now();
    
    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);
    
    if (!_delegate->startRender(*this))
    {
        return;
    }
    
    _selectionMattingRenderPass->setObjectsToRender(_world->selection());
    
    FrameSubmission submission { _delegate.get() };
    if (!submission.isValid()) {
        return;
    }
    
    for (auto pass : _renderPasses)
    {
        pass->updateBuffersState();
        pass->updateUniforms(*this);
        
        pass->willStartRender(*this);
    }
    
    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
        auto end = HighResClock::now();
        const auto dT = end - now;
        
        const float renderFrameTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(dT).count();
        
        for (auto pass : _renderPasses)
        {
            pass->onCompletedCommandBuffer(*this, renderFrameTimeInMs);
        }
        
        dispatch_semaphore_signal(block_sema);
    }];
    
    for (auto pass : _renderPasses)
    {
        pass->render(*this, commandBuffer);
    }
    
    _delegate->presentDrawable(commandBuffer);
    
    [commandBuffer commit];
    
    submission.end();
    
    if (_renderCallback != nullptr)
    {
        _renderCallback(*this);
    }
}

Ray
Renderer::ray(float2 pixelPosition) const
{
    const auto size = cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    const auto p = pixelToNDC(size, pixelPosition);
    
    const auto ray = Ray::make(p, _sdfRenderPass->viewDependentUniforms().cameraUniforms[kLeftCameraIndex]);
    return ray;
}

PickResult
Renderer::pick(float2 pixelPosition) const
{
    const auto pixel = renderPixel(kLeftCameraIndex, pixelPosition);
    
    const float mattingZ = renderMatting(kLeftCameraIndex, pixelPosition);
    
    const auto& uniforms = _sdfRenderPass->viewDependentUniforms();
    const auto& cameraUniforms = uniforms.cameraUniforms[kLeftCameraIndex];
    const auto& serializedWorld = uniforms.serializedWorldObject[kLeftCameraIndex];
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return ::pickObject(p, cameraUniforms, serializedWorld, materials);
}

float4
Renderer::renderPixel(size_t cameraIndex, float2 pixelPosition) const
{
    const auto& uniforms = _sdfRenderPass->viewDependentUniforms();
    const auto& cameraUniforms = uniforms.cameraUniforms[kLeftCameraIndex];
    const auto& serializedWorld = uniforms.serializedWorldObject[kLeftCameraIndex];
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return renderDefault(p, cameraUniforms, serializedWorld, materials).color;
}

float
Renderer::renderMatting(size_t cameraIndex, float2 pixelPosition) const
{
    const auto& uniforms = _sdfRenderPass->viewDependentUniforms();
    const auto& cameraUniforms = uniforms.cameraUniforms[kLeftCameraIndex];
    const auto& serializedWorld = uniforms.serializedWorldObject[kLeftCameraIndex];
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    const auto res = render<MattingShader, true /*write to depth*/>(p, cameraUniforms, serializedWorld, materials);
    
    return cameraUniforms.inverseZ() ? (1.f - res.depth) : res.depth;
}


void
Renderer::invalidate()
{
    _delegate->invalidate();
}

void
Renderer::pause()
{
    _delegate->pause();
}

id<MTLDevice>
Renderer::mtlDevice() const
{
    return _commandQueue.device;
}

AppleImage*
Renderer::renderImage() const
{
    auto rgbColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear);
    
    const auto size = cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    std::vector<uint32_t> buffer;
    buffer.resize(size.x * size.y);
    
    for (float y=0; y < size.y; ++y)
    {
        for (float x=0; x < size.x; ++x)
        {
            uint32_t& pixel = buffer[(y * size.x) + x];
            const float2 pixelCoordinates { x, y };
            
            const auto fragment = renderPixel(kLeftCameraIndex, pixelCoordinates);
            
            const uint8_t r = clamp(fragment.r, 0.f, 1.f) * 255.f;
            const uint8_t g = clamp(fragment.g, 0.f, 1.f) * 255.f;
            const uint8_t b = clamp(fragment.b, 0.f, 1.f) * 255.f;
            
            uint32_t v = r | (g << 8) | (b << 16) | 0xFF000000;
            pixel = v;
        }
    }
    
    const size_t bytesPerRow = size.x * sizeof(uint32_t);
    CGContextRef context = CGBitmapContextCreateWithData(buffer.data(),
                                  size.x,
                                  size.y,
                                  8,
                                  bytesPerRow,
                                  rgbColorSpace,
                                    kCGImageAlphaPremultipliedLast,
                                  nullptr,
                                  nullptr
                                  );
    
    CGColorSpaceRelease(rgbColorSpace);

    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    
    
    
#if TARGET_OS_OSX
    AppleImage* img = [[AppleImage alloc] initWithCGImage:cgImage size:CGSizeMake(size.x, size.y)];
#else
    AppleImage* img = [[UIImage alloc] initWithCGImage:cgImage];
#endif
    
    CGImageRelease(cgImage);
    
    return img;
}
