//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#pragma once

#import <Metal/Metal.h>
#import <TargetConditionals.h>
#include <memory>

class RenderTargetConfiguration
{
public:
    using Ptr = std::shared_ptr<RenderTargetConfiguration>;
    using CPtr = std::shared_ptr<const RenderTargetConfiguration>;
    
    virtual ~RenderTargetConfiguration() = default;
    
#if TARGET_OS_SIMULATOR
    static constexpr MTLPixelFormat kDefaultColorPixelFormat = MTLPixelFormatBGRA8Unorm;
#else
    static constexpr MTLPixelFormat kDefaultColorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
#endif
    
    MTLPixelFormat colorPixelFormat = kDefaultColorPixelFormat;
    MTLPixelFormat depthPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    NSUInteger sampleCount = 1;
};
