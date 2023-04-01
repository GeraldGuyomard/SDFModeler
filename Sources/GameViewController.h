//
//  GameViewController.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//


#import "CommonDefinitions.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "Renderer.h"
#import "Object3D.h"

// Our macOS view controller.
@interface GameViewController : ViewControllerBase

+(GameViewController*)instance;

@property(readonly, nonatomic) Renderer* renderer;
@property(readonly, nonatomic) const World& world;

@end
