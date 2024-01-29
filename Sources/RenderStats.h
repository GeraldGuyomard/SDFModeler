//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include <cstdio>
#import <simd/simd.h>

class RenderStats final
{
public:
    
    RenderStats() = default;
    void submitFrameRenderTime(float time);
    
    void setViewportSize(simd_float2);
    
private:
    
    float _frameTimeAccumulation = 0.f;
    size_t _nbAccumulatedFrames = 0.f;
    simd_float2 _viewportSize = {0.f, 0.f};
    
    static constexpr size_t kNbFramesForFPSComputation = 30.f;
    size_t _nbFramesForFPSComputation = kNbFramesForFPSComputation;
};


