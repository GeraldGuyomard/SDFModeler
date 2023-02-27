//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <MetalKit/MetalKit.h>

simd_float3 translation(simd_float4x4 m);
void setTranslation(simd_float4x4& m, simd_float3 t);

simd_float3 direction(simd_float4x4 m);
void setDirection(simd_float4x4& m, simd_float3 t);

simd_float3 up(simd_float4x4 m);
void setUp(simd_float4x4& m, simd_float3 t);

void decompose(simd_float4x4 m, simd_float3& oRight, simd_float3& oUp, simd_float3& oForward, simd_float3& position);
simd_float4x4 recompose(simd_float3 right, simd_float3 up, simd_float3 forward, simd_float3 position);

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@property(nonatomic) simd_float4x4 cameraTransform;

@end

