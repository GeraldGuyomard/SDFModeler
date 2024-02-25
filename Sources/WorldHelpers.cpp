//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "WorldHelpers.h"

WorldPtr makeDefaultWorld()
{
    auto world = World::make();
    auto rootObject = world->rootObject();
    
    auto white = world->addMaterial(float4 { 1, 1, 1, 1 });
    
    constexpr float kZ = 0;
    constexpr float s = 0.5f;
    float3 pos = float3 {0.5, 0, kZ};
    
    auto whiteSphere = std::make_shared<TObject3D<SDFSphere>>(world, SDFSphere { 0.6f });
    whiteSphere->setLocalTransform(RSTTransformer { float3 { 0, 1, -0.1f } } );
    whiteSphere->setMaterial(white);
    
    rootObject->addChild(whiteSphere);
    
    auto red = world->addMaterial(float4 { 1, 0, 0, 1 });
    auto redSphere = std::make_shared<TObject3D<SDFSphere>>(world, SDFSphere { 0.5f });
    redSphere->setLocalTransform(RSTTransformer { float3 { -1, 0, kZ } } );
    redSphere->setMaterial(red);
    
    rootObject->addChild(redSphere);
    
    auto yellow = world->addMaterial(float4 { 1, 1, 0, 1 });
    auto roundedYellowBox = std::make_shared<TObject3D<SDFRoundedBox>>(world,
                                                                       SDFRoundedBox { float3 { 0.4f, 0.6f, 0.4f }, 0.1f });
    roundedYellowBox->setLocalTransform(RSTTransformer { pos - float3 { 0.5, 0, 0 }, float3 {1, 1, 0}, degToRad(45.f), s });
    roundedYellowBox->setMaterial(yellow);
    rootObject->addChild(roundedYellowBox);
    
    auto whiteBoxHalf = std::make_shared<TObject3D<SDFBox>>(world,
                                                            SDFBox { float3 { 0.4f * s, 0.6f * s, 0.4f * s } });
    whiteBoxHalf->setLocalTransform(RSTTransformer { pos + float3 { 0.5, 0, 0 } , float3 {1, 1, 0}, degToRad(45.f) });
    whiteBoxHalf->setMaterial(white);
    rootObject->addChild(whiteBoxHalf);
    

    auto blue = world->addMaterial(float4 { 0, 0, 1, 1 });
    
    auto blueSphere = std::make_shared<TObject3D<SDFSphere>>(world, SDFSphere { 0.4f });
    //blueSphere->setOperation(SDFOperation::substraction);
    blueSphere->setLocalTransform(RSTTransformer { pos });
    blueSphere->setMaterial(blue);
    rootObject->addChild(blueSphere);
 
    auto green = world->addMaterial(float4 { 0, 1, 0, 1 });
    
    auto greenSphere = std::make_shared<TObject3D<SDFSphere>>(world, SDFSphere { 0.45f });
    greenSphere->setLocalTransform(RSTTransformer { float3 { -1, 1, kZ } });
    greenSphere->setMaterial(green);
    rootObject->addChild(greenSphere);
    
    const float3 compositionOrigin { -2., 0.f, kZ + 0.5f };
    
    auto spherePart = std::make_shared<TObject3D<SDFSphere>>(world, SDFSphere { 0.4f });
    spherePart->setLocalTransform(RSTTransformer { float3 { 0, 0.6f, 0 } });
    
    auto boxPart = std::make_shared<TObject3D<SDFRoundedBox>>(world, SDFRoundedBox
        { float3 { 0.2, 0.4, 0.2 }, 0.1 }
    );
    boxPart->setLocalTransform(RSTTransformer { float3 { 0, 0, 0 } });
    
    auto negativeSpherePart = std::make_shared<TObject3D<SDFSphere>>(world, SDFSphere { 0.4f });
    negativeSpherePart->setLocalTransform(RSTTransformer { float3 { 0, 0.25, 0.5f } });
    negativeSpherePart->setOperation(SDFOperation::substraction);
    
    auto negativeRoundedBoxPart = std::make_shared<TObject3D<SDFRoundedBox>>(world, SDFRoundedBox
    { float3 { 0.1f, 0.1f, 0.3f }, 0.05f });
    negativeRoundedBoxPart->setLocalTransform(RSTTransformer { float3 { 0, -0.5f, 0.5f }, float3 {0, 0, 1}, degToRad(45.f) });
    negativeRoundedBoxPart->setOperation(SDFOperation::substraction);
    
    auto sdfUnionMaterial = world->addMaterial(float4 { 0, 1, 1, 1 });
    auto sdfUnion = std::make_shared<Object3D>(world);
    sdfUnion->setMaterial(sdfUnionMaterial);
    sdfUnion->setLocalTransform(matrix4x4_translation(compositionOrigin));
    sdfUnion->setIsCompound(true);
    
    sdfUnion->addChild(spherePart);
    sdfUnion->addChild(boxPart);
    sdfUnion->addChild(negativeSpherePart);
    sdfUnion->addChild(negativeRoundedBoxPart);
    
    rootObject->addChild(sdfUnion);
    
    return world;
}
