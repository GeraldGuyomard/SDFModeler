//
//  Shaders.metal
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

// File for Metal kernel and shader functions


#include "World.h"

struct VertexShaderOut
{
    float4 position [[position]];
    float2 viewportNDC;
};

vertex VertexShaderOut vertexShader(Vertex in [[stage_in]],
                                    constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]])
{
    VertexShaderOut out;

    out.position = in.position;
    out.viewportNDC = in.viewportNDC;

    return out;
}

fragment float4 fragmentShader(VertexShaderOut in [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(BufferIndexUniforms) ]],
                               constant DynamicScene& mutableState [[ buffer(BufferIndexDynamicScenes) ]])
{
    PhongShader shader(uniforms.lightDirection);
    //CellShader shader;
    
    return render(in.viewportNDC, shader, uniforms, mutableState);
}
