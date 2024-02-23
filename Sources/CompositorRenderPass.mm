//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "CompositorRenderPass.h"

PipelineConfiguration::Ptr
BlurRenderPass::makePipelineConfiguration(id<MTLLibrary> _Nonnull mtlLib) const
{
    auto config = _inherited::makePipelineConfiguration(mtlLib);
    
    config->pipelineName = "Blur Render";
    
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderBlur"];
    config->depthEnabled = false;
    
    return config;
}
