//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#pragma once

#import "Renderer.h"

class OutlineRenderPass : public SDFRenderPass
{
public:
    
private:
    void configure(EncodingContext&) const override;
};
