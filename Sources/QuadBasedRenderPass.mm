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

PipelineConfiguration
QuadBasedRenderPass::pipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    PipelineConfiguration config;
    config.vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderSDF"];
    return config;
}


bool
QuadBasedRenderPass::init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config)
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
    
    _quadVertexBuffer = [device newBufferWithBytes:&s_Vertices length:sizeof(s_Vertices)
                                             options:MTLResourceStorageModeShared];
    
    _quadVertexBuffer.label = @"QuadVertexBuffer";
    
    return true;
}


void
QuadBasedRenderPass::render(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    auto renderEncoder = makeRenderEncoder(renderer, cmdBuffer);
    
    if (renderEncoder != nil)
    {
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder pushDebugGroup:@"RayMarch"];
        [renderEncoder setCullMode:MTLCullModeNone];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        if (_depthEnabled)
        {
            [renderEncoder setDepthStencilState:_depthState];
        }
        
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
