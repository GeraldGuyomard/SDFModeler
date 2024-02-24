//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "SelectionOutlineRenderPass.h"
#include "Renderer.h"

namespace
{
    Vertex s_Vertices[4] = {
        { {-1.f, +1.f , 0.0f, 1.f}, {0.f, 0.f} },
        { {-1.f, -1.f , 0.0f, 1.f}, {0.f, 1.f} },
        { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 0.f} },
        { {+1.f, -1.f , 0.0f, 1.f}, {1.f, 1.f} }
    };
}

id<MTLRenderCommandEncoder>_Nullable
SelectionOutlineRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [renderer.delegate()->currentRenderPassDescriptor() copy];
    
    auto colorAttachment = renderPassDescriptor.colorAttachments[0];
    colorAttachment.loadAction = MTLLoadActionLoad;
    colorAttachment.storeAction = MTLStoreActionStore;
    //colorAttachment.clearColor = MTLClearColorMake(1, 0, 0, 1);
    
    auto depthAttachment = renderPassDescriptor.depthAttachment;
    depthAttachment.loadAction = MTLLoadActionDontCare;
    depthAttachment.storeAction = MTLStoreActionDontCare;
    
    if (renderPassDescriptor != nullptr)
    {
        auto encoder = [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        encoder.label = @"SelectionOutlineRenderPass";
        return encoder;
    }
    else
    {
        return nullptr;
    }
}

PipelineConfiguration::Ptr
SelectionOutlineRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = std::make_unique<PipelineConfiguration>();
    
    config->pipelineName = "Selection Outline";
    
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
    
    config->vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderOutline"];
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderOutline"];
    
    config->blendEnabled = true;
    config->depthWriteEnabled = false;
    
    return config;
}

void
SelectionOutlineRenderPass::setMattingTextureProvider(const MattingTextureProvider& provider)
{
    _mattingTextureProvider = provider;
}

bool
SelectionOutlineRenderPass::init(Renderer& renderer)
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
SelectionOutlineRenderPass::updateBuffersState()
{
    _uniformsBuffer->update();
}

void
SelectionOutlineRenderPass::updateUniforms(Renderer& renderer)
{
    auto& uniforms = _uniformsBuffer->uniform();
}


void
SelectionOutlineRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    if (_mattingTextureProvider == nullptr)
    {
        return;
    }
    
    auto inputTexture = _mattingTextureProvider();
    
    if (inputTexture == nullptr)
    {
        return;
    }
    
    // Draw a quad on screen
    [encoder setVertexBuffer:_quadVertexBuffer
                            offset:0
                           atIndex:BufferIndexMeshPositions];
    
    [encoder setVertexBuffer:_quadVertexBuffer
                            offset:0
                           atIndex:BufferIndexUVs];
    
    [encoder setFragmentTexture:inputTexture atIndex:TextureIndexInput];
    
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}
