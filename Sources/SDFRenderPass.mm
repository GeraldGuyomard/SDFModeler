//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "SDFRenderPass.h"
#include "Renderer.h"

Vertex s_Vertices[4] = {
    { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
    { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
    { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
    { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
};

PipelineConfiguration
SDFRenderPass::pipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    PipelineConfiguration config;
    config.pipelineName = "SDF Render";
    
    config.vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderSDF"];
    config.fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderSDF"];
    config.depthEnabled = true;
    
    return config;
}


bool
SDFRenderPass::init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config)
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
    
    const auto renderConfig = pipelineConfiguration(mtlLib);
    _depthEnabled = renderConfig.depthEnabled;
    
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = [NSString stringWithUTF8String:renderConfig.pipelineName.c_str()];
    pipelineStateDescriptor.rasterSampleCount = config.sampleCount;
    pipelineStateDescriptor.vertexFunction = renderConfig.vertexFunction;
    pipelineStateDescriptor.fragmentFunction = renderConfig.fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = config.colorPixelFormat;
    
    if (_depthEnabled)
    {
        pipelineStateDescriptor.depthAttachmentPixelFormat = config.depthStencilPixelFormat;
        pipelineStateDescriptor.stencilAttachmentPixelFormat = config.depthStencilPixelFormat;
    }
    
    NSError *error = NULL;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    if (_depthEnabled)
    {
        MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
        depthStateDesc.depthWriteEnabled = YES;
        _depthState = [device newDepthStencilStateWithDescriptor:depthStateDesc];
    }
    
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
        configure(context);
        
        world->encode(context, serializedMaterials);
    }
}

id <MTLRenderCommandEncoder> _Nullable
SDFRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    auto renderPassDescriptor = renderer.delegate()->currentRenderPassDescriptor();
    
    if (renderPassDescriptor != nullptr)
    {
        return [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    }
    else
    {
        return nullptr;
    }
}

void
SDFRenderPass::render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    const float2 viewportSize = renderer.renderSize();
    
    const auto& serialized = _serializedWorldBuffer->uniform();
    const float2 tileGridSize { serialized.numTileColumns, serialized.numTileRows };
    
    _renderStats.setViewportInfo(viewportSize, tileGridSize);
    
    auto renderEncoder = makeRenderEncoder(renderer, cmdBuffer);
    
    if (renderEncoder != nil)
    {
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder pushDebugGroup:@"RayMarch"];
        
        //[renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        //[renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setCullMode:MTLCullModeNone];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        if (_depthEnabled)
        {
            [renderEncoder setDepthStencilState:_depthState];
        }
        
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
