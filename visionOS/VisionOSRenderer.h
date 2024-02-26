//
//  RenderStats.hpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#pragma once

#import <Foundation/Foundation.h>
#import <CompositorServices/CompositorServices.h>

@interface VisionOSRenderer : NSObject

- (instancetype) initWithLayerRenderer:(cp_layer_renderer_t)renderer;

- (void)startRenderLoop;
- (void)renderImage;

+(void) renderOnCPU;


@end


