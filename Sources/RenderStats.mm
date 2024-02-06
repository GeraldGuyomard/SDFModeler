//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "RenderStats.h"
#import <Foundation/Foundation.h>
#include "CommonDefinitions.h"

void
RenderStats::setViewportInfo(const float2& viewportSize, const float2& tileGridSize)
{
    if (!equals(_viewportSize, viewportSize))
    {
        _viewportSize = viewportSize;
        _tileGridSize = tileGridSize;
        
        _reset();
    }
}

void 
RenderStats::_reset()
{
    _accumulatedTime = 0.f;
    _nbAccumulatedFrames = 0;
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(_snapshotInterval * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        
        if (_nbAccumulatedFrames != 0)
        {
            const float averageTime = _accumulatedTime / _nbAccumulatedFrames;
            const float fps = 1000.f / averageTime;
            
            printf("(%d, %d) pixels, tile grid (%d, %d), Render Time=%5.2f FPS=%3.1f\n", int(_viewportSize.x), int(_viewportSize.y), int(_tileGridSize.x), int(_tileGridSize.y), averageTime, fps);
        }
        else
        {
            //printf("(%d, %d) Render Time=N/A FPS=N/A\n", int(_viewportSize.x), int(_viewportSize.y));
        }
        
        _reset();
    });
}

void
RenderStats::submitFrameRenderTime(float time)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        
        if (_firstSubmission) {
            _reset();
            _firstSubmission = false;
        }
        
        _accumulatedTime += time;
        ++_nbAccumulatedFrames;
    });

}
