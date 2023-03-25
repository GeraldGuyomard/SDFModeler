//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>

#import "Renderer.h"

// Include header shared between C code here, which executes Metal API commands, and .metal files
#import "ShaderTypes.h"

#include "Scene.h"
#include "FragmentShader/PhongShader.h"

#include "Object3D.h"

constexpr NSUInteger kMaxBuffersInFlight = 3;

constexpr size_t kAlignedUniformsSize = (sizeof(Uniforms) & ~0xFF) + 0x100;
constexpr size_t kAlignedSerializedScenesSize = (sizeof(SerializedScene) & ~0xFF) + 0x100;

Vertex s_Vertices[4] = {
    { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
    { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
    { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
    { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
};

@implementation Renderer
{
    dispatch_semaphore_t _inFlightSemaphore;
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _dynamicUniformBuffer;
    id <MTLBuffer> _dynamicSerializedSceneBuffer;
    
    id <MTLBuffer> _quadVertexBuffer;
    id <MTLRenderPipelineState> _pipelineState;
    id <MTLDepthStencilState> _depthState;
    MTLVertexDescriptor *_mtlVertexDescriptor;

    uint32_t _uniformBufferOffset;
    uint8_t _uniformBufferIndex;
    void* _uniformBufferAddress;

    uint32_t _serializedSceneBufferOffset;
    uint8_t _serializedSceneBufferIndex;
    void* _serializedSceneBufferAddress;
    
    float4x4 _projectionMatrix;
    float4x4 _invProjectionMatrix;
    
    float4x4 _cameraTransform;
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self)
    {
        _device = view.device;
        _inFlightSemaphore = dispatch_semaphore_create(kMaxBuffersInFlight);
        [self _loadMetalWithView:view];
        
        _cameraTransform = matrix4x4_translation(float3 {0, 0, 5.f});
    }

    return self;
}

- (void)_loadMetalWithView:(nonnull MTKView *)view;
{
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

    {
        const NSUInteger bufferSize = kAlignedUniformsSize * kMaxBuffersInFlight;

        _dynamicUniformBuffer = [_device newBufferWithLength:bufferSize
                                                     options:MTLResourceStorageModeShared];

        _dynamicUniformBuffer.label = @"UniformBuffer";
    }

    {
        const NSUInteger bufferSize = kAlignedSerializedScenesSize * kMaxBuffersInFlight;

        _dynamicSerializedSceneBuffer = [_device newBufferWithLength:bufferSize
                                                     options:MTLResourceStorageModeShared];

        _dynamicSerializedSceneBuffer.label = @"SerializedSceneBuffer";
    }

    
    _quadVertexBuffer = [_device newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    //_quadVertexBuffer
    _commandQueue = [_device newCommandQueue];
}

- (void)_updateDynamicBufferState
{
    /// Update the state of our uniform buffers before rendering

    _uniformBufferIndex = (_uniformBufferIndex + 1) % kMaxBuffersInFlight;
    _uniformBufferOffset = kAlignedUniformsSize * _uniformBufferIndex;
    _uniformBufferAddress = ((uint8_t*)_dynamicUniformBuffer.contents) + _uniformBufferOffset;
    
    _serializedSceneBufferIndex = (_serializedSceneBufferIndex + 1) % kMaxBuffersInFlight;
    _serializedSceneBufferOffset = kAlignedSerializedScenesSize * _uniformBufferIndex;
    _serializedSceneBufferAddress = ((uint8_t*)_dynamicSerializedSceneBuffer.contents) + _serializedSceneBufferOffset;
}

- (float4x4)cameraTransform
{
    return _cameraTransform;
}

- (void)setCameraTransform:(float4x4)cameraTransform
{
    _cameraTransform = cameraTransform;
}

-(const Uniforms*) uniforms
{
    return (Uniforms*)_uniformBufferAddress;
}

-(const SerializedScene*) mutableState
{
    return (SerializedScene*)_serializedSceneBufferAddress;
}

- (void)_updateGameState
{
    /// Update any game state before encoding renderint commands to our drawable

    Uniforms& uniforms = *((Uniforms*)_uniformBufferAddress);

    uniforms.invProjectionMatrix = _invProjectionMatrix;
    uniforms.cameraMatrix = _cameraTransform;
    uniforms.ndcToWorldTransform = uniforms.cameraMatrix * uniforms.invProjectionMatrix;
    uniforms.lightDirection = float3 { -1, -1, -1 };
    
    SerializedScene* serializedScene = ((SerializedScene*) _serializedSceneBufferAddress);
    
    World world;
    
    auto whiteSphere = std::make_unique<TObject3D<Sphere>>(Sphere { { 0.6f }, { float3 { 0, 1, -0.1f } }, { float4 { 1, 1, 1, 1 } } });
    world.addObject(std::move(whiteSphere));
    
    constexpr float kZ = 0;
    
    auto redSphere = std::make_unique<TObject3D<Sphere>>(Sphere { { 0.5f }, { float3 { -1, 0, kZ } }, { float4 { 1, 0, 0, 1 } } });
    world.addObject(std::move(redSphere));
    
    float3 pos = float3 {0.5, 0, kZ};
    
    constexpr float s = 0.5f;
    
    auto roundedYellowBox = std::make_unique<TObject3D<RoundedBox>>(RoundedBox
    {
        { float3 { 0.4f, 0.6f, 0.4f }, 0.1f }, // geometry
        { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s }, // transform
        { float4 { 1, 1, 0, 1 } } // material
    });
    world.addObject(std::move(roundedYellowBox));
    
    auto whiteBoxHalf = std::make_unique<TObject3D<Box>>(Box
    {
        { float3 { 0.4f * s, 0.6f * s, 0.4f * s } }, // geometry
        { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) }, // transform
        { float4 { 1, 1, 1, 1 } } // material
    });
    world.addObject(std::move(whiteBoxHalf));
    
    auto blueSphere = std::make_unique<TObject3D<Sphere>>(Sphere
    {
        { 0.4f },
        { pos },
        { float4 { 0, 0, 1, 1 } }
    });
    world.addObject(std::move(blueSphere));
    
    auto greenSphere = std::make_unique<TObject3D<Sphere>>(Sphere
    {
        { 0.45f },
        { float3 { -1, 1, kZ } },
        { float4 { 0, 1, 0, 1 } }
    });
    world.addObject(std::move(greenSphere));
    
    /*
    Sphere spherePart {  { 0.4f }, // geom
        { float3 { -2., 0.6, kZ + 0.5f } } // transform
    };
    
    RoundedBox boxPart {
        { float3 { 0.2, 0.4, 0.2 }, 0.1 }, // geometry
        { float3 { -2., 0, kZ + 0.5f } } // transform
    };
    
    Sphere negativeSpherePart {  { 0.4f }, // geom
        { float3 { -2., 0.3, kZ + 1.f } } // transform
    };
    
    using TUnion = SDFUnion<Sphere, RoundedBox>;
    TUnion sdfUnion(spherePart, boxPart);
    
    using TComposite = SDFSubstraction<TUnion, Sphere>;
    TComposite composite(sdfUnion, negativeSpherePart);
    
    SDFObject<TComposite, RSTTransformer, ConstMaterial> uni( composite, {}, { float4 { 0, 1, 1, 1 } } );
*/
    
    world.serialize(*serializedScene);
}

- (void)drawInMTKView:(nonnull MTKView *)view
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

    [self _updateDynamicBufferState];

    [self _updateGameState];

    /// Delay getting the currentRenderPassDescriptor until we absolutely need it to avoid
    ///   holding onto the drawable and blocking the display pipeline any longer than necessary
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;

    if(renderPassDescriptor != nil) {

        /// Final pass rendering code here

        id <MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";

        [renderEncoder pushDebugGroup:@"DrawBox"];

        //[renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        //[renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setCullMode:MTLCullModeNone];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        [renderEncoder setDepthStencilState:_depthState];

        [renderEncoder setFragmentBuffer:_dynamicUniformBuffer
                                  offset:_uniformBufferOffset
                                 atIndex:BufferIndexUniforms];

        [renderEncoder setFragmentBuffer:_dynamicSerializedSceneBuffer
                                  offset:_serializedSceneBufferOffset
                                 atIndex:BufferIndexSerializedScenes];

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
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    /// Respond to drawable size or orientation changes here

    float aspect = size.width / (float)size.height;
    const float farZ = 100.f;
    _projectionMatrix = matrix_perspective_right_hand(45.0f * (M_PI / 180.0f), aspect, 0.1f, farZ);
    _invProjectionMatrix = simd_inverse(_projectionMatrix);
}

@end

