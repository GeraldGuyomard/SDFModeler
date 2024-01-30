//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#include <cstdio>
#import "CommonDefinitions.h"


class RenderStats final
{
public:
    
    RenderStats() = default;
    void submitFrameRenderTime(float time);
    
    void setViewportSize(simd_float2);
    
private:
    
    void _reset();
    
    float _accumulatedTime = 0.f;
    size_t _nbAccumulatedFrames = 0.f;
    simd_float2 _viewportSize = {0.f, 0.f};
    bool _firstSubmission = true;
    
    static constexpr float kDefaultSnapshotInterval = 0.25f;
    float _snapshotInterval = kDefaultSnapshotInterval;
};


