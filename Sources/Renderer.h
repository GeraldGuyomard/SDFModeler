//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <Metal/Metal.h>
#import "CommonDefinitions.h"

#import "Uniforms.h"
#import "ShaderTypes.h"

#include "SerializedWorldObject.h"
#include "Camera.h"
#include "RenderStats.h"

#include <functional>

template <typename TUniform, BufferIndex bufferIndex, size_t TMaxBuffersInFlight>
class TUniformBuffer final
{
public:
    TUniformBuffer(id<MTLDevice> _Nonnull device, NSString* _Nonnull label)
    {
        const NSUInteger bufferSize = alignedSize() * TMaxBuffersInFlight;
        _mtlBuffer = [device newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
        _mtlBuffer.label = label;
    }
    
    TUniform& uniform()
    {
        return *_uniform;
    }
    
    void update()
    {
        _index = (_index + 1) % TMaxBuffersInFlight;
        _offset = alignedSize() * _index;
        _uniform = reinterpret_cast<TUniform*>(((uint8_t*)_mtlBuffer.contents) + _offset);
    }
    
    void setFragmentBuffer(id <MTLRenderCommandEncoder> _Nonnull encoder)
    {
        [encoder setFragmentBuffer:_mtlBuffer offset:_offset atIndex:bufferIndex];
    }
    
private:
    
    static constexpr size_t alignedSize()
    {
        return (sizeof(TUniform) & ~0xFF) + 0x100;
    }
    
    id <MTLBuffer> _Nonnull _mtlBuffer;
    
    uint32_t _offset = 0;
    uint8_t _index = 0;
    TUniform* _Nullable _uniform = nullptr;
};

class Renderer;

class RendererDelegateConfiguration final
{
public:
    MTLPixelFormat depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    MTLPixelFormat colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    NSUInteger sampleCount = 1;
};

class RendererDelegate
{
public:
    using Ptr = std::unique_ptr<RendererDelegate>;
    virtual ~RendererDelegate() = default;
    
    virtual bool init(Renderer*_Nonnull renderer) = 0;
    
    virtual RendererDelegateConfiguration configuration() const = 0;
    virtual id<MTLDevice> _Nonnull getMTLDevice() const = 0;
    
    virtual float2 renderSize() const = 0;
    virtual float2 renderSizeInPoints() const = 0;
    
    virtual MTLRenderPassDescriptor* _Nullable currentRenderPassDescriptor() const = 0;
    virtual id <MTLDrawable> _Nonnull currentDrawable() const = 0;
    
    virtual void invalidate() = 0;
    virtual void pause() = 0;    
};

class RenderPass
{
public:
    using Ptr = std::unique_ptr<RenderPass>;
    
    virtual ~RenderPass() = default;
    
    virtual bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RendererDelegateConfiguration& config) = 0;
    
    virtual void updateBuffersState() = 0;
    virtual void updateUniforms(Renderer&) = 0;
    
    virtual void render(Renderer&, id <MTLRenderCommandEncoder>_Nullable renderEncoder) = 0;
    
    virtual void onCompletedCommandBuffer(float renderDuration) {}
    
    static constexpr size_t kMaxBuffersInFlight = 3;
    
private:
    MTLVertexDescriptor* _Nonnull _mtlVertexDescriptor;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    id <MTLRenderPipelineState> _Nonnull _pipelineState;
    id <MTLDepthStencilState> _Nonnull _depthState;
};

class SDFRenderPass : public RenderPass
{
public:
    bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RendererDelegateConfiguration& config) override;
    void updateBuffersState() override;
    void updateUniforms(Renderer&) override;
    void render(Renderer&, id <MTLRenderCommandEncoder>_Nullable renderEncoder) override;
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
};

class Renderer final
{
public:
    Renderer(RendererDelegate::Ptr);
    ~Renderer();
    
    float2 renderSize() const;
    
    Camera::Ptr camera() const { return _camera; }
    void setCamera(const Camera::Ptr&);
    
    WorldPtr world() const { return _world; }
    void setWorld(const WorldPtr&);
    
    using RenderCallback = std::function<void(Renderer&)>;
    void setRenderCallback(const RenderCallback&);
    
    Ray ray(float2 pixelPosition) const;
    PickResult pick(float2 pixelPosition) const;
    float4 renderPixel(float2 pixelPosition) const;
    
    void invalidate();
    
public:
    void render();
    void updateCameraTransforms();
    void pause();
    
public:
    const float4x4& projectionMatrix() const { return _projectionMatrix; }
    const float4x4& invProjectionMatrix() const { return _invProjectionMatrix; }
    
private:
    
    void init();
    
    void updateBuffersState();
    void updateUniforms();
    
    Camera::Ptr _camera;
    WorldPtr _world;
    
    RendererDelegate::Ptr _delegate;
    
    dispatch_semaphore_t _Nonnull _inFlightSemaphore;
    
    id <MTLCommandQueue> _Nonnull _commandQueue;

    std::vector<RenderPass*> _renderPasses;
    std::unique_ptr<SDFRenderPass> _sdfRenderPass;

    
    float4x4 _projectionMatrix;
    float4x4 _invProjectionMatrix;
    
    RenderCallback _renderCallback;
};
