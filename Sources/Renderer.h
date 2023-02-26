//
//  Renderer.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <MetalKit/MetalKit.h>

matrix_float4x4 matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ);
matrix_float4x4 matrix4x4_translation(float tx, float ty, float tz);
matrix_float4x4 matrix4x4_rotation(float radians, vector_float3 axis);
matrix_float4x4 matrix4x4_rotation(float radians, vector_float3 axis, vector_float3 origin);

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

