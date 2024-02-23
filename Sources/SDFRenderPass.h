//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#include "RenderPass.h"
#include "SerializedWorldObject.h"
#include "ShaderTypes.h"
#include "TUniformBuffer.h"
#include "RenderStats.h"

class EncodingContext;

class SDFRenderPass : public RenderPass
{
public:
    using _inherited = RenderPass;
    
    bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config) override;
    void updateBuffersState() override;
    void updateUniforms(Renderer&) override;
    void render(Renderer&, id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    void onCompletedCommandBuffer(float renderDuration) override;
    
    const Uniforms& uniforms() const
    {
        return _uniformsBuffer->uniform();
    }

    const SerializedWorldObject& serializedWorld() const
    {
        return _serializedWorldBuffer->uniform();
    }

    const Materials& materials() const
    {
        return _materialsBuffer->uniform();
    }
    
protected:
    
    PipelineConfiguration pipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const override;
    id <MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer,  id<MTLCommandBuffer> _Nonnull cmdBuffer) override;
    
    virtual void configure(EncodingContext&) const {}
    
private:
    MTLVertexDescriptor* _Nonnull _mtlVertexDescriptor;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    id <MTLRenderPipelineState> _Nonnull _pipelineState;
    id <MTLDepthStencilState> _Nonnull _depthState;
    
    using UniformsBuffer = TUniformBuffer<Uniforms, BufferIndex::BufferIndexUniforms, kMaxBuffersInFlight>;
    std::unique_ptr<UniformsBuffer> _uniformsBuffer;
    
    using SerializedWorldBuffer = TUniformBuffer<SerializedWorldObject, BufferIndex::BufferIndexSerializedWorld, kMaxBuffersInFlight>;
    std::unique_ptr<SerializedWorldBuffer> _serializedWorldBuffer;

    using SerializedMaterials = TUniformBuffer<Materials, BufferIndex::BufferIndexMaterials, kMaxBuffersInFlight>;
    std::unique_ptr<SerializedMaterials> _materialsBuffer;
    
    RenderStats _renderStats;
    bool _depthEnabled = false;
    
};

