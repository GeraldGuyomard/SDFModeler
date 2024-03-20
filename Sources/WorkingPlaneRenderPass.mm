//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "WorkingPlaneRenderPass.h"
#include "Renderer.h"



WorkingPlaneRenderPass::WorkingPlaneRenderPass()
{}

id<MTLRenderCommandEncoder>_Nullable
WorkingPlaneRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [renderer.delegate()->renderPassDescriptor(kLeftCameraIndex) copy];
    
    auto colorAttachment = renderPassDescriptor.colorAttachments[0];
    colorAttachment.loadAction = MTLLoadActionDontCare;
    colorAttachment.storeAction = MTLStoreActionStore;
    
    auto depthAttachment = renderPassDescriptor.depthAttachment;
    depthAttachment.loadAction = MTLLoadActionDontCare;
    depthAttachment.storeAction = MTLStoreActionStore;
    
    if (renderPassDescriptor != nullptr)
    {
        auto encoder = [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        encoder.label = @"WorkingPlaneRenderPass";
        return encoder;
    }
    else
    {
        return nullptr;
    }
}

PipelineConfiguration::Ptr
WorkingPlaneRenderPass::makePipelineConfiguration(Renderer& renderer) const
{
    auto config = std::make_unique<PipelineConfiguration>();
    
    config->pipelineName = "Working Plane";
    
    config->vertexDescriptor = [[MTLVertexDescriptor alloc] init];

    config->vertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat4;
    config->vertexDescriptor.attributes[VertexAttributePosition].offset = offsetof(VertexShader_SelectionOutlineIn, position);
    config->vertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    config->vertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    config->vertexDescriptor.attributes[VertexAttributeTexcoord].offset = offsetof(VertexShader_SelectionOutlineIn, textCoords);
    config->vertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = BufferIndexUVs;

    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stride = sizeof(VertexShader_SelectionOutlineIn);
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stride = sizeof(VertexShader_SelectionOutlineIn);
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepFunction = MTLVertexStepFunctionPerVertex;
    
    auto mtlLib = renderer.mtlLibrary();
    config->vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderOutline"];
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderOutline"];
    
    auto presentationConfig = renderer.delegate()->presentConfiguration();
    config->colorPixelFormat = presentationConfig->colorPixelFormat;
    config->blendEnabled = true;
    
    config->depthCompareFunction = MTLCompareFunctionAlways;
    
    return config;
}

bool
WorkingPlaneRenderPass::init(Renderer& renderer)
{
    if (!_inherited::init(renderer))
    {
        return false;
    }
    
    auto device = renderer.mtlDevice();
    _quadVertexBuffer = [device newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    _uniformsBuffer = std::make_unique<UniformsBuffer>(device, @"OutlineUniformsBuffer");
    
    return true;
}

void
WorkingPlaneRenderPass::updateBuffersState()
{
    _uniformsBuffer->update();
}

void
WorkingPlaneRenderPass::updateUniforms(Renderer& renderer)
{
    auto& uniforms = _uniformsBuffer->uniform();
    
    const auto size = renderer.cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    uniforms.viewportSize = size;
    
    const float contentScaleFactor = renderer.delegate()->contentScaleFactor();
    const float thickness = contentScaleFactor * _thickness;
    uniforms.samplingDelta = float2 { thickness, thickness };
    
    uniforms.color = _color;
    
}

void
WorkingPlaneRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    _setupViewports(renderer, encoder);
    
    _uniformsBuffer->setFragmentBuffer(encoder);
    
    // Draw a quad on screen
    [encoder setVertexBuffer:_quadVertexBuffer
                            offset:0
                           atIndex:BufferIndexMeshPositions];
    
    [encoder setVertexBuffer:_quadVertexBuffer
                            offset:0
                           atIndex:BufferIndexUVs];
    
    
    auto descriptor = renderer.delegate()->renderPassDescriptor(kLeftCameraIndex);
    auto mainDepth = descriptor.depthAttachment.texture;
    
    if (auto rateMap = descriptor.rasterizationRateMap)
    {
        auto device = renderer.mtlDevice();
        
        MTLSizeAndAlign rateMapParamSize = rateMap.parameterBufferSizeAndAlign;
        id<MTLBuffer> rateMapDataBuffer = [device newBufferWithLength: rateMapParamSize.size options:MTLResourceStorageModeShared];

        // Copy the rate map's data into the buffer.
        [rateMap copyParameterDataToBuffer:rateMapDataBuffer offset:0];
        
        [encoder setFragmentBuffer:rateMapDataBuffer offset:0 atIndex:BufferIndexRasterizationRateMapUniforms];
    }
    
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}
