//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <MetalKit/MetalKit.h>
#import "CommonDefinitions.h"
#import "Uniforms.h"
#include "Scene.h"
#include "Camera.h"

@class RendererMTKViewDelegate;

class Renderer final
{
public:
    Renderer(MTKView* _Nonnull);
    ~Renderer();
    
    float2 renderSize() const;
    
    Camera::Ptr camera() const { return _camera; }
    void setCamera(const Camera::Ptr&);
    
    const Uniforms& uniforms() const;
    const SerializedWorld& serializedWorld() const;
    
public:
    void render();
    void onRenderSizeChanged(const CGSize&);
    
private:
    
    void init();
    
    void updateDynamicBufferState();
    void updateUniforms();
    
    Camera::Ptr _camera;
    RendererMTKViewDelegate* _Nonnull _mtkViewDelegate;
    
    const __weak MTKView* _Nullable _mtkView;
    const id <MTLDevice> _Nonnull _device;
    
    dispatch_semaphore_t _Nonnull _inFlightSemaphore;
    
    id <MTLCommandQueue> _Nonnull _commandQueue;

    id <MTLBuffer> _Nonnull _dynamicUniformBuffer;
    id <MTLBuffer> _Nonnull _dynamicSerializedWorldBuffer;
    
    id <MTLBuffer> _Nonnull _quadVertexBuffer;
    id <MTLRenderPipelineState> _Nonnull _pipelineState;
    id <MTLDepthStencilState> _Nonnull _depthState;
    MTLVertexDescriptor* _Nonnull _mtlVertexDescriptor;

    uint32_t _uniformBufferOffset = 0;
    uint8_t _uniformBufferIndex = 0;
    void* _Nullable _uniformBufferAddress = nullptr;

    uint32_t _serializedWorldBufferOffset = 0;
    uint8_t _serializedWorldBufferIndex = 0;
    void* _Nullable _serializedWorldBufferAddress = nullptr;
    
    float4x4 _projectionMatrix;
    float4x4 _invProjectionMatrix;
};
