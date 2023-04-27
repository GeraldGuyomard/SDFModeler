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

#include "MainViewController.h"

Vertex s_Vertices[4] = {
    { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
    { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
    { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
    { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
};

@interface RendererMTKViewDelegate : NSObject<MTKViewDelegate>

- (instancetype) initWithRenderer:(Renderer*)renderer;
- (void)terminate;

- (void)delayPause;

@end

@implementation RendererMTKViewDelegate
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

Renderer::Renderer(MTKView* _Nonnull view)
: _mtkView(view),
_device(view.device),
_inFlightSemaphore(dispatch_semaphore_create(kMaxBuffersInFlight))
{
    _mtkViewDelegate = [[RendererMTKViewDelegate alloc] initWithRenderer:this];
    _mtkView.delegate = _mtkViewDelegate;
    _mtkView.paused = YES;
    
    init();
    updateCameraTransforms();
}

Renderer::~Renderer()
{
    [_mtkViewDelegate terminate];
}

float2
Renderer::renderSize() const
{
    CAMetalLayer* layer = (CAMetalLayer*) _mtkView.layer;
    const CGSize size = layer.drawableSize;
    return float2 { float(size.width), float(size.height) };
}

void
Renderer::init()
{
    auto view = _mtkView;
    
    /// Load Metal state objects and initialize renderer dependent view properties

    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    view.sampleCount = 1;

    _mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

    _mtlVertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat4;
    _mtlVertexDescriptor.attributes[VertexAttributePosition].offset = offsetof(Vertex, position);
    _mtlVertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    _mtlVertexDescriptor.attributes[VertexAttributeViewportNDC].format = MTLVertexFormatFloat2;
    _mtlVertexDescriptor.attributes[VertexAttributeViewportNDC].offset = offsetof(Vertex, viewportNDC);
    _mtlVertexDescriptor.attributes[VertexAttributeViewportNDC].bufferIndex = BufferIndexMeshViewportNDCs;

    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stride = sizeof(Vertex);
    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    _mtlVertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stride = sizeof(Vertex);
    _mtlVertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepRate = 1;
    _mtlVertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepFunction = MTLVertexStepFunctionPerVertex;

    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];

    id <MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];

    id <MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"MyPipeline";
    pipelineStateDescriptor.rasterSampleCount = view.sampleCount;
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    pipelineStateDescriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = view.depthStencilPixelFormat;

    NSError *error = NULL;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    _depthState = [_device newDepthStencilStateWithDescriptor:depthStateDesc];

    _uniformsBuffer = std::make_unique<UniformsBuffer>(_device, @"UniformBuffer");
    _serializedWorldBuffer = std::make_unique<SerializedWorldBuffer>(_device, @"SerializedSceneBuffer");
    _materialsBuffer = std::make_unique<SerializedMaterials>(_device, @"Materials");

    _quadVertexBuffer = [_device newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    //_quadVertexBuffer
    _commandQueue = [_device newCommandQueue];
}


void
Renderer::updateBuffersState()
{
    _uniformsBuffer->update();
    _serializedWorldBuffer->update();
    _materialsBuffer->update();
}

void Renderer::setCamera(const Camera::Ptr& cam)
{
    _camera = cam;
    
    if (_camera != nullptr)
    {
        updateCameraTransforms();
    }
}


const Uniforms&
Renderer::uniforms() const
{
    return _uniformsBuffer->uniform();
}

const SerializedWorldObject&
Renderer::serializedWorld() const
{
    return _serializedWorldBuffer->uniform();
}

const Materials&
Renderer::materials() const
{
    return _materialsBuffer->uniform();
}

void
Renderer::updateUniforms()
{
    /// Update any game state before encoding renderint commands to our drawable
    auto& uniforms = _uniformsBuffer->uniform();

    const float4x4 cameraMatrix = (_camera != nullptr) ? _camera->worldTransform() : float4x4_identity();
    
    uniforms.invProjectionMatrix = _invProjectionMatrix;
    uniforms.ndcToWorldTransform = cameraMatrix * uniforms.invProjectionMatrix;
    
    uniforms.lightDirection = float3 { -1, -1, -1 };
    
    if (auto world = this->world())
    {
        auto& serializedWorld = _serializedWorldBuffer->uniform();
        auto& serializedMaterials = _materialsBuffer->uniform();
        
        const auto viewMatrix = inverse(cameraMatrix);
        const auto viewProjectionMatrix = _projectionMatrix * viewMatrix;
        
        world->serialize(viewProjectionMatrix, serializedWorld, serializedMaterials);
    }
}

void
Renderer::setRenderCallback(const RenderCallback& cb)
{
    _renderCallback = cb;
}

void
Renderer::render()
{
    /// Per frame updates here
    
    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);
    
    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
        dispatch_semaphore_signal(block_sema);
    }];
    
    updateBuffersState();
    updateUniforms();
    
    auto view = _mtkView;
    
    /// Delay getting the currentRenderPassDescriptor until we absolutely need it to avoid
    ///   holding onto the drawable and blocking the display pipeline any longer than necessary
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if(renderPassDescriptor != nil)
    {
        /// Final pass rendering code here
        
        id <MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder pushDebugGroup:@"RayMarch"];
        
        //[renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        //[renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setCullMode:MTLCullModeNone];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        [renderEncoder setDepthStencilState:_depthState];
        
        _uniformsBuffer->setFragmentBuffer(renderEncoder);
        _serializedWorldBuffer->setFragmentBuffer(renderEncoder);
        _materialsBuffer->setFragmentBuffer(renderEncoder);
        
        // Draw a quad on screen
        [renderEncoder setVertexBuffer:_quadVertexBuffer
                                offset:0
                               atIndex:BufferIndexMeshPositions];
        
        [renderEncoder setVertexBuffer:_quadVertexBuffer
                                offset:0
                               atIndex:BufferIndexMeshViewportNDCs];
        
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        
        [renderEncoder popDebugGroup];
        
        [renderEncoder endEncoding];
        
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    
    [commandBuffer commit];
    
    if (_renderCallback != nullptr)
    {
        _renderCallback(*this);
    }
}

void
Renderer::updateCameraTransforms()
{
    if (_camera != nullptr)
    {
        const CGSize size = _mtkView.bounds.size;
        
        const float2 s { float(size.width), float(size.height) };
        _camera->setViewportSize(s);
        
        _projectionMatrix = _camera->computeProjectionMatrix();
        _invProjectionMatrix = simd_inverse(_projectionMatrix);
    }
}

Ray
Renderer::ray(float2 pixelPosition) const
{
    const auto size = renderSize();
    const auto p = pixelToNDC(size, pixelPosition);
    
    const auto ray = Ray::make(p, uniforms());
    return ray;
}

PickResult
Renderer::pick(float2 pixelPosition) const
{
    const auto& uniforms = this->uniforms();
    const auto& serializedWorld = this->serializedWorld();
    const auto& materials = this->materials();
    
    const auto size = renderSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return ::pickObject(p, uniforms, serializedWorld, materials);
}

float4
Renderer::renderPixel(float2 pixelPosition) const
{
    const auto& uniforms = this->uniforms();
    const auto& serializedWorld = this->serializedWorld();
    const auto& materials = this->materials();
    
    const auto size = renderSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return renderDefault(p, uniforms, serializedWorld, materials);
}

void
Renderer::setWorld(const WorldPtr& world)
{
    _world = world;
    
    invalidate();
}

void
Renderer::invalidate()
{
    _mtkView.paused = NO;
    [_mtkViewDelegate delayPause];
}

void
Renderer::pause()
{
    _mtkView.paused = YES;
}
