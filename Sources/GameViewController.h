//
//  GameViewController.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "Renderer.h"

// Our macOS view controller.
@interface GameViewController : NSViewController

+(GameViewController*)instance;

@property(readonly, nonatomic) Renderer* renderer;

@end
