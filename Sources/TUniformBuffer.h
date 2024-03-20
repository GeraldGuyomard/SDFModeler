//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import <Metal/Metal.h>
#import "CommonDefinitions.h"
#include "ShaderTypes.h"

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
    
    id <MTLBuffer> mtlBuffer() const
    {
        return _mtlBuffer;
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

template <BufferIndex bufferIndex, size_t TMaxBuffersInFlight>
class TVariableSizeUniformBuffer final
{
public:
    TVariableSizeUniformBuffer(id<MTLDevice> _Nonnull device, size_t size, NSString* _Nonnull label)
    {
        _allocate(device, size, label);
    }
    
    void* _Nullable uniform()
    {
        return _uniform;
    }
    
    void reallocIfNeeded(size_t newSize)
    {
        const size_t newAlignedSize = alignedSize(newSize);
        
        if (newAlignedSize > _alignedSize)
        {
            _allocate(_mtlBuffer.device, newAlignedSize, _mtlBuffer.label);
            
            _index = 0;
            _offset = 0;
            _uniform = reinterpret_cast<void*>(((uint8_t*)_mtlBuffer.contents) + _offset);
        }
    }
    
    void update()
    {
        _index = (_index + 1) % TMaxBuffersInFlight;
        _offset = _alignedSize * _index;
        _uniform = reinterpret_cast<void*>(((uint8_t*)_mtlBuffer.contents) + _offset);
    }
    
    void setFragmentBuffer(id <MTLRenderCommandEncoder> _Nonnull encoder)
    {
        [encoder setFragmentBuffer:_mtlBuffer offset:_offset atIndex:bufferIndex];
    }
    
    id <MTLBuffer> _Nonnull mtlBuffer() const
    {
        return _mtlBuffer;
    }
    
    uint32_t offset() const
    {
        return _offset;
    }
    
private:
    
    void _allocate(id<MTLDevice> _Nonnull device, size_t size, NSString* _Nullable label)
    {
        _alignedSize = alignedSize(size);
        
        const NSUInteger bufferSize = _alignedSize * TMaxBuffersInFlight;
        _mtlBuffer = [device newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
        _mtlBuffer.label = label;
    }
    
    static size_t alignedSize(size_t size)
    {
        return (size & ~0xFF) + 0x100;
    }
    
    id <MTLBuffer> _Nonnull _mtlBuffer;
    
    uint32_t _offset = 0;
    uint8_t _index = 0;
    size_t _alignedSize = 0;
    
    void* _Nullable _uniform = nullptr;
};
