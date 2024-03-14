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

SelectionOutlineRenderPass::SelectionOutlineRenderPass()
:
_color { 252.0f / 255.0f, 202.0f / 255.0f, 0.0f, 1.f },
_thickness(2.f)
{}

id<MTLRenderCommandEncoder>_Nullable
SelectionOutlineRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    MTLRenderPassDescriptor* renderPassDescriptor = [renderer.delegate()->renderPassDescriptor(kLeftCameraIndex) copy];
    
    auto colorAttachment = renderPassDescriptor.colorAttachments[0];
    colorAttachment.loadAction = MTLLoadActionLoad;
    colorAttachment.storeAction = MTLStoreActionStore;
    
    auto depthAttachment = renderPassDescriptor.depthAttachment;
    depthAttachment.loadAction = MTLLoadActionLoad;
    depthAttachment.storeAction = MTLStoreActionStore;
    
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
SelectionOutlineRenderPass::makePipelineConfiguration(Renderer& renderer) const
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
    
    auto mtlLib = renderer.mtlLibrary();
    config->vertexFunction = [mtlLib newFunctionWithName:@"vertexShaderOutline"];
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderOutline"];
    
    auto presentationConfig = renderer.delegate()->presentConfiguration();
    config->colorPixelFormat = presentationConfig->colorPixelFormat;
    config->blendEnabled = true;
    
    config->depthCompareFunction = MTLCompareFunctionAlways;
    
    return config;
}

void
SelectionOutlineRenderPass::setDepthTextureProvider(const DepthTextureProvider& provider)
{
    _depthTextureProvider = provider;
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
    enable(!renderer.world()->selection().empty());
    
    if (!enabled())
    {
        return;
    }
    
    auto& uniforms = _uniformsBuffer->uniform();
    
    const auto size = renderer.cameraRig()->cameras()[kLeftCameraIndex]->viewportSize();
    
    const float contentScaleFactor = renderer.delegate()->contentScaleFactor();
    const float thickness = contentScaleFactor * _thickness;
    uniforms.samplingDelta = float2 { thickness, thickness } / size;
    
    uniforms.color = _color;
}


void
SelectionOutlineRenderPass::_render(Renderer& renderer, id<MTLRenderCommandEncoder> _Nonnull encoder)
{
    if (_depthTextureProvider == nullptr)
    {
        return;
    }

    auto inputDepth = _depthTextureProvider();
    
    if (inputDepth == nil)
    {
        return;
    }
    
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
    
    [encoder setFragmentTexture:mainDepth atIndex:MainDepthTextureIndex];
    [encoder setFragmentTexture:inputDepth atIndex:MattingDepthIndexInput];
    
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}

void 
SelectionOutlineRenderPass::setThickness(float t)
{
    _thickness = t;
}

void
SelectionOutlineRenderPass::setColor(float4 c)
{
    _color = c;
}
