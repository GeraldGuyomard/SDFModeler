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
#include "OutlineRenderPass.h"

#include "MainViewController.h"

Vertex s_Vertices[4] = {
    { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
    { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
    { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
    { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
};

id<MTLFunction>
SDFRenderPass::vertexFunction(id<MTLLibrary> _Nonnull mtlLib) const
{
    return [mtlLib newFunctionWithName:@"vertexShaderSDF"];
}

id<MTLFunction>
SDFRenderPass::fragmentFunction(id<MTLLibrary> _Nonnull mtlLib) const
{
    return [mtlLib newFunctionWithName:@"fragmentShaderSDF"];
    //return [mtlLib newFunctionWithName:@"fragmentShaderMatting"];
}

bool
SDFRenderPass::init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RendererDelegateConfiguration& config)
{
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
    
    id <MTLFunction> vertexFunc = vertexFunction(mtlLib);
    id <MTLFunction> fragmentFunc = fragmentFunction(mtlLib);
    
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"MyPipeline";
    pipelineStateDescriptor.rasterSampleCount = config.sampleCount;
    pipelineStateDescriptor.vertexFunction = vertexFunc;
    pipelineStateDescriptor.fragmentFunction = fragmentFunc;
    pipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = config.colorPixelFormat;
    pipelineStateDescriptor.depthAttachmentPixelFormat = config.depthStencilPixelFormat;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = config.depthStencilPixelFormat;

    NSError *error = NULL;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    _depthState = [device newDepthStencilStateWithDescriptor:depthStateDesc];
    
    _uniformsBuffer = std::make_unique<UniformsBuffer>(device, @"UniformBuffer");
    _serializedWorldBuffer = std::make_unique<SerializedWorldBuffer>(device, @"SerializedSceneBuffer");
    _materialsBuffer = std::make_unique<SerializedMaterials>(device, @"Materials");

    _quadVertexBuffer = [device newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    return true;
}

void
SDFRenderPass::updateBuffersState()
{
    _uniformsBuffer->update();
    _serializedWorldBuffer->update();
    _materialsBuffer->update();
}

void 
SDFRenderPass::updateUniforms(Renderer& renderer)
{
    auto& uniforms = _uniformsBuffer->uniform();

    const auto camera = renderer.camera();
    
    const float4x4 cameraMatrix = (camera != nullptr) ? camera->worldTransform() : float4x4_identity();
    
    uniforms.viewportSize = renderer.renderSize();
    uniforms.ndcToWorldTransform = cameraMatrix * renderer.invProjectionMatrix();
    uniforms.worldTransformToNdc = inverse(uniforms.ndcToWorldTransform);
    
    uniforms.lightDirection = float3 { -1, -1, -1 };
    
    if (auto world = renderer.world())
    {
        auto& serializedWorld = _serializedWorldBuffer->uniform();
        auto& serializedMaterials = _materialsBuffer->uniform();
        
        const auto viewMatrix = inverse(cameraMatrix);
        const auto viewProjectionMatrix = renderer.projectionMatrix() * viewMatrix;
        
        EncodingContext context { world, viewProjectionMatrix, uniforms.viewportSize, serializedWorld };
        
        world->encode(context, serializedMaterials);
    }
}

void
SDFRenderPass::render(Renderer& renderer, id <MTLRenderCommandEncoder>_Nullable renderEncoder)
{
    const float2 viewportSize = renderer.renderSize();
    
    const auto& serialized = _serializedWorldBuffer->uniform();
    const float2 tileGridSize { serialized.numTileColumns, serialized.numTileRows };
    
    _renderStats.setViewportInfo(viewportSize, tileGridSize);
    
    if(renderEncoder != nil)
    {
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
    }
}

void 
SDFRenderPass::onCompletedCommandBuffer(float renderDuration)
{
    _renderStats.submitFrameRenderTime(renderDuration);
}

Renderer::Renderer(RendererDelegate::Ptr delegate)
: _delegate(std::move(delegate)),
_inFlightSemaphore(dispatch_semaphore_create(RenderPass::kMaxBuffersInFlight))
{
    _delegate->init(this);
    
    init();
    updateCameraTransforms();
}

Renderer::~Renderer() = default;

float2
Renderer::renderSize() const
{
    return _delegate->renderSize();
}

void
Renderer::init()
{
    _sdfRenderPass = std::make_unique<SDFRenderPass>();
    _outlineRenderPass = std::make_unique<OutlineRenderPass>();
    
    _renderPasses.push_back(_sdfRenderPass.get());
    
    const auto device = _delegate->getMTLDevice();
    id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
    const auto config = _delegate->configuration();
    
    for (auto renderPass : _renderPasses)
    {
        renderPass->init(device, defaultLibrary, config);
    }
    
    //_quadVertexBuffer
    _commandQueue = [device newCommandQueue];
}


void
Renderer::updateBuffersState()
{
    for (auto pass : _renderPasses)
    {
        pass->updateBuffersState();
    }
}

void Renderer::setCamera(const Camera::Ptr& cam)
{
    _camera = cam;
    
    if (_camera != nullptr)
    {
        updateCameraTransforms();
    }
}

void
Renderer::updateUniforms()
{
    for (auto pass : _renderPasses)
    {
        pass->updateUniforms(*this);
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
    auto now = HighResClock::now();
    
    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);
    
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
            pass->onCompletedCommandBuffer(renderFrameTimeInMs);
        }
        
        dispatch_semaphore_signal(block_sema);
    }];
    
    updateBuffersState();
    updateUniforms();
    
    auto renderPassDescriptor =_delegate->currentRenderPassDescriptor();
    
    for (auto pass : _renderPasses)
    {
        id <MTLRenderCommandEncoder> renderEncoder = nil;
        
        if (renderPassDescriptor != nullptr)
        {
            renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        }
        //renderEncoder.label = @"MyRenderEncoder";
        
        pass->render(*this, renderEncoder);
    }
    
    auto drawable = _delegate->currentDrawable();
    [commandBuffer presentDrawable:drawable];
    
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
        const auto s = _delegate->renderSizeInPoints();
        
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
    
    const auto ray = Ray::make(p, _sdfRenderPass->uniforms());
    return ray;
}

PickResult
Renderer::pick(float2 pixelPosition) const
{
    const auto pixel = renderPixel(pixelPosition);
    
    const auto& uniforms = _sdfRenderPass->uniforms();
    const auto& serializedWorld = _sdfRenderPass->serializedWorld();
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = renderSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return ::pickObject(p, uniforms, serializedWorld, materials);
}

float4
Renderer::renderPixel(float2 pixelPosition) const
{
    const auto& uniforms = _sdfRenderPass->uniforms();
    const auto& serializedWorld = _sdfRenderPass->serializedWorld();
    const auto& materials = _sdfRenderPass->materials();
    
    const auto size = renderSize();
    
    const auto p = pixelToNDC(size, pixelPosition);
    
    return renderDefault(p, uniforms, serializedWorld, materials).color;
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
    _delegate->invalidate();
}

void
Renderer::pause()
{
    _delegate->pause();
}
