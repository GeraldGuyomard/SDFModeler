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
