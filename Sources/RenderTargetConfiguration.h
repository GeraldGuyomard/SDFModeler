//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#pragma once

#import <Metal/Metal.h>
#include <memory>

class RenderTargetConfiguration final
{
public:
    using Ptr = std::shared_ptr<const RenderTargetConfiguration>;
    using CPtr = std::shared_ptr<const RenderTargetConfiguration>;
    
    MTLPixelFormat depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    MTLPixelFormat colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    NSUInteger sampleCount = 1;
};
