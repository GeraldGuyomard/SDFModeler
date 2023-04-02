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
    
    CGSize renderSize() const;
    
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
    
    const __weak MTKView* _mtkView;
    const id <MTLDevice> _device;
    
    dispatch_semaphore_t _inFlightSemaphore;
    
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _dynamicUniformBuffer;
    id <MTLBuffer> _dynamicSerializedWorldBuffer;
    
    id <MTLBuffer> _quadVertexBuffer;
    id <MTLRenderPipelineState> _pipelineState;
    id <MTLDepthStencilState> _depthState;
    MTLVertexDescriptor *_mtlVertexDescriptor;

    uint32_t _uniformBufferOffset = 0;
    uint8_t _uniformBufferIndex = 0;
    void* _uniformBufferAddress = nullptr;

    uint32_t _serializedWorldBufferOffset = 0;
    uint8_t _serializedWorldBufferIndex = 0;
    void* _serializedWorldBufferAddress = nullptr;
    
    float4x4 _projectionMatrix;
    float4x4 _invProjectionMatrix;
};
