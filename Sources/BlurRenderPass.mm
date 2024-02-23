//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "BlurRenderPass.h"
#include "Renderer.h"

/*
namespace
{
    Vertex s_Vertices[4] = {
        { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
        { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
        { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
        { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
    };
}
*/

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
BlurRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    
    if (_targetTexture == nil)
    {
        const auto size = renderer.renderSize();
        
        const auto colorPixelFormat = renderer.delegate()->configuration()->colorPixelFormat;
        auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorPixelFormat width:NSUInteger(size.x) height:NSUInteger(size.y) mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageRenderTarget;
        
        _targetTexture = [renderer.mtlDevice() newTextureWithDescriptor:textureDescriptor];
    }
    
    renderPassDescriptor.colorAttachments[0].texture = _targetTexture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionDontCare;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    
    return [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
}

PipelineConfiguration::Ptr
BlurRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = std::make_unique<PipelineConfiguration>();
    
    config->pipelineName = "Blur";
    
    config->vertexDescriptor = [[MTLVertexDescriptor alloc] init];

    config->vertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat4;
    config->vertexDescriptor.attributes[VertexAttributePosition].offset = offsetof(VertexShader_BlurIn, position);
    config->vertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    config->vertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    config->vertexDescriptor.attributes[VertexAttributeTexcoord].offset = offsetof(VertexShader_BlurIn, textCoords);
    config->vertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = BufferIndexUVs;

    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stride = sizeof(VertexShader_BlurIn);
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stride = sizeof(VertexShader_BlurIn);
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepFunction = MTLVertexStepFunctionPerVertex;
    
    config->vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderBlur"];
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderBlur"];
    
    config->depthEnabled = false;
    
    return config;
}

void
BlurRenderPass::setInputTextureProvider(const InputTextureProvider& provider)
{
    _inputTextureProvider = provider;
}

bool
BlurRenderPass::init(Renderer& renderer)
{
    if (!_inherited::init(renderer))
    {
        return false;
    }
    
    auto device = renderer.mtlDevice();
    _quadVertexBuffer = [device newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    return true;
}

void
BlurRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    if (_inputTextureProvider == nullptr)
    {
        return;
    }
    
    auto inputTexture = _inputTextureProvider();
    
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
