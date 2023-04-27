//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <MetalKit/MetalKit.h>
#import "CommonDefinitions.h"

#import "Uniforms.h"
#import "ShaderTypes.h"

#include "SerializedWorldObject.h"
#include "Camera.h"

#include <functional>

@class RendererMTKViewDelegate;

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

class Renderer final
{
public:
    Renderer(MTKView* _Nonnull);
    ~Renderer();
    
    float2 renderSize() const;
    
    Camera::Ptr camera() const { return _camera; }
    void setCamera(const Camera::Ptr&);
    
    WorldPtr world() const { return _world; }
    void setWorld(const WorldPtr&);
    
    const Uniforms& uniforms() const;
    const SerializedWorldObject& serializedWorld() const;
    const Materials& materials() const;
    
    using RenderCallback = std::function<void(Renderer&)>;
    void setRenderCallback(const RenderCallback&);
    
    Ray ray(float2 pixelPosition) const;
    PickResult pick(float2 pixelPosition) const;
    float4 renderPixel(float2 pixelPosition) const;
    
    void invalidate();
    
public:
    void render();
    void updateCameraTransforms();
    
private:
    
    static constexpr size_t kMaxBuffersInFlight = 3;
    
    void init();
    
    void updateBuffersState();
    void updateUniforms();
    
    Camera::Ptr _camera;
    WorldPtr _world;
    
    RendererMTKViewDelegate* _Nonnull _mtkViewDelegate;
    
    const __weak MTKView* _Nullable _mtkView;
    const id <MTLDevice> _Nonnull _device;
    
    dispatch_semaphore_t _Nonnull _inFlightSemaphore;
    
    id <MTLCommandQueue> _Nonnull _commandQueue;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    id <MTLRenderPipelineState> _Nonnull _pipelineState;
    id <MTLDepthStencilState> _Nonnull _depthState;
    MTLVertexDescriptor* _Nonnull _mtlVertexDescriptor;

    using UniformsBuffer = TUniformBuffer<Uniforms, BufferIndex::BufferIndexUniforms, kMaxBuffersInFlight>;
    std::unique_ptr<UniformsBuffer> _uniformsBuffer;
    
    using SerializedWorldBuffer = TUniformBuffer<SerializedWorldObject, BufferIndex::BufferIndexSerializedWorld, kMaxBuffersInFlight>;
    std::unique_ptr<SerializedWorldBuffer> _serializedWorldBuffer;

    using SerializedMaterials = TUniformBuffer<Materials, BufferIndex::BufferIndexMaterials, kMaxBuffersInFlight>;
    std::unique_ptr<SerializedMaterials> _materialsBuffer;
    
    float4x4 _projectionMatrix;
    float4x4 _invProjectionMatrix;
    
    RenderCallback _renderCallback;
};
