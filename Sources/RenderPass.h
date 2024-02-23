//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import <Metal/Metal.h>
#include <memory>
#include <string>

class RenderPassConfiguration final
{
public:
    MTLPixelFormat depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    MTLPixelFormat colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    NSUInteger sampleCount = 1;
};

class Renderer;

struct PipelineConfiguration final
{
    id <MTLFunction> _Nonnull vertexFunction;
    id <MTLFunction> _Nonnull fragmentFunction;
    bool depthEnabled = true;
    
    std::string pipelineName;
};

class RenderPass
{
public:
    using Ptr = std::unique_ptr<RenderPass>;
    
    virtual ~RenderPass() = default;
    
    virtual bool init(id<MTLDevice> _Nonnull device, id<MTLLibrary> _Nonnull mtlLib, const RenderPassConfiguration& config) = 0;
    
    virtual void updateBuffersState() = 0;
    virtual void updateUniforms(Renderer&) = 0;
    
    virtual void prepareRender(Renderer&) {}
    virtual void render(Renderer& renderer,
                        id<MTLCommandBuffer> _Nonnull cmdBuffer) = 0;
    
    virtual void onCompletedCommandBuffer(float renderDuration) {}
    
    static constexpr size_t kMaxBuffersInFlight = 3;
    
protected:
    
    virtual PipelineConfiguration pipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const = 0;
    virtual id<MTLRenderCommandEncoder>_Nullable makeRenderEncoder(Renderer& renderer,  id<MTLCommandBuffer> _Nonnull cmdBuffer) = 0;
};
