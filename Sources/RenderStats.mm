//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "RenderStats.h"
#import <Foundation/Foundation.h>

void 
RenderStats::setViewportSize(simd_float2 viewportSize)
{
    _viewportSize = viewportSize;
}

void
RenderStats::submitFrameRenderTime(float time)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        _frameTimeAccumulation += time;
        _nbAccumulatedFrames++;
        if (_nbAccumulatedFrames == _nbFramesForFPSComputation)
        {
            const float averageTime = _frameTimeAccumulation / _nbFramesForFPSComputation;
            const float fps = 1000.f / averageTime;
            
            _frameTimeAccumulation = 0.f;
            _nbAccumulatedFrames = 0;
            
            printf("(%d, %d) Render Time=%5.2f FPS=%3.1f\n", int(_viewportSize.x), int(_viewportSize.y), averageTime, fps);
        }
    });

}
