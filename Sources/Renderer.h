//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <MetalKit/MetalKit.h>
#import "CommonDefinitions.h"
#import "Uniforms.h"

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@property(nonatomic) float4x4 cameraTransform;
@property(nonatomic, readonly, nonnull) const Uniforms* uniforms;
@property(nonatomic, readonly, nonnull) const DynamicScene* mutableState;

@end


