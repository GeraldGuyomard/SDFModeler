//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "OutlineRenderPass.h"

void
OutlineRenderPass::configure(EncodingContext& ctx) const
{
    ctx.setOptionFlags(EncodingContext::fRenderSelectedObjectsOnly);
}
