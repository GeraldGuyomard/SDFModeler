//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#include "QuadBasedRenderPass.h"
#include "Renderer.h"

namespace
{
    Vertex s_Vertices[4] = {
        { {-1.f, +1.f , 0.0f, 1.f}, {-1.f, 1.f} },
        { {-1.f, -1.f , 0.0f, 1.f}, {-1.f, -1.f} },
        { {+1.f, +1.f , 0.0f, 1.f}, {1.f, 1.f} },
        { {+1.f, -1.f , 0.0f, 1.f}, {1.f, -1.f} }
    };
}

PipelineConfiguration::Ptr
QuadBasedRenderPass::makePipelineConfiguration(const RenderTargetConfiguration::CPtr& presentationConfig, id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = std::make_unique<PipelineConfiguration>();
    
    config->vertexDescriptor = [[MTLVertexDescriptor alloc] init];

    config->vertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat4;
    config->vertexDescriptor.attributes[VertexAttributePosition].offset = offsetof(Vertex, position);
    config->vertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    config->vertexDescriptor.attributes[VertexAttributeViewportNDC].format = MTLVertexFormatFloat2;
    config->vertexDescriptor.attributes[VertexAttributeViewportNDC].offset = offsetof(Vertex, viewportNDC);
    config->vertexDescriptor.attributes[VertexAttributeViewportNDC].bufferIndex = BufferIndexMeshViewportNDCs;

    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stride = sizeof(Vertex);
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stride = sizeof(Vertex);
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepRate = 1;
    config->vertexDescriptor.layouts[BufferIndexMeshViewportNDCs].stepFunction = MTLVertexStepFunctionPerVertex;
    
    config->vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderSDF"];
    
    return config;
}


bool
QuadBasedRenderPass::init(Renderer& renderer)
{
    if (!_inherited::init(renderer))
    {
        return false;
    }
    
    _quadVertexBuffer = [renderer.mtlDevice() newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    return true;
}

void
QuadBasedRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    // Draw a quad on screen
    [encoder setVertexBuffer:_quadVertexBuffer
                            offset:0
                           atIndex:BufferIndexMeshPositions];
    
    [encoder setVertexBuffer:_quadVertexBuffer
                            offset:0
                           atIndex:BufferIndexMeshViewportNDCs];
    
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}
