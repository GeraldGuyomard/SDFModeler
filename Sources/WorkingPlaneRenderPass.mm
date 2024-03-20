//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "WorkingPlaneRenderPass.h"
#include "Renderer.h"


namespace
{
    constexpr float kPlaneHalfSize = 1.f;

    Vertex s_Vertices[4] = {
        { {-kPlaneHalfSize, 0.f, +kPlaneHalfSize, 1.f}, {0.f, 1.f} },
        { {-kPlaneHalfSize, 0.f, -kPlaneHalfSize, 1.f}, {0.f, 0.f} },
        { {+kPlaneHalfSize, 0.f, +kPlaneHalfSize, 1.f}, {1.f, 1.f} },
        { {+kPlaneHalfSize, 0.f, -kPlaneHalfSize, 1.f}, {1.f, 0.f} }
    };
}

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
    config->vertexDescriptor.attributes[VertexAttributePosition].offset = offsetof(VertexShader_WorkingPlaneIn, position);
    config->vertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    config->vertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    config->vertexDescriptor.attributes[VertexAttributeTexcoord].offset = offsetof(VertexShader_WorkingPlaneIn, textCoords);
    config->vertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = BufferIndexUVs;

    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stride = sizeof(VertexShader_WorkingPlaneIn);
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stride = sizeof(VertexShader_WorkingPlaneIn);
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepFunction = MTLVertexStepFunctionPerVertex;
    
    auto mtlLib = renderer.mtlLibrary();
    config->vertexFunction = [mtlLib newFunctionWithName:@"vertexShader_WorkingPlane"];
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShader_WorkingPlane"];
    
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
    
    _quadVertexBuffer.label = @"WorkingPlaneVertexBuffer";
    
    const size_t s = sizeof(WorkingPlaneUniforms);
    _uniformsBuffer = std::make_unique<UniformsBuffer>(device, @"WorkingPlaneUniformsBuffer");
    const size_t s2 = _uniformsBuffer->mtlBuffer().length;
    
    //_gridTransform = matrix4x4_translation(float3 { 0.f, -1.f ,0.f} );
    _gridTransform = float4x4_identity();
    
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
    
    const auto& cameras = renderer.cameraRig()->cameras();
    const size_t cameraCount = cameras.size();
    
    for (size_t i=0; i < cameraCount; ++i)
    {
        const auto& camera = cameras[i];
        const auto viewMatrix = inverse(camera->worldTransform());
        
        uniforms.projViewModelMatrix[i] =  camera->projectionMatrix() * viewMatrix * _gridTransform;
        
        const float2 viewportSize = camera->viewportSize();
        
        RectF bounds;
        
        for (size_t j=0; j < 4; ++j)
        {
            const float4 pos = s_Vertices[j].position;
            float4 pos2d = uniforms.projViewModelMatrix[i] * pos;
            float3 p = pos2d.xyz / pos2d.w;
            const float x = ((p.x + 1.f) * 0.5f) * viewportSize.x;
            const float y = ((-p.y + 1.f) * 0.5f) * viewportSize.y;
            
            bounds.add({x, y});
        }
    }
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
    
    
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}
