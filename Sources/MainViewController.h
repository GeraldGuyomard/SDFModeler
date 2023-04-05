//
//  MainViewController.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//


#import "CommonDefinitions.h"

#import "Renderer.h"
#import "Object3D.h"
#import "CameraInteraction.h"

// Our macOS view controller.
@interface MainViewController : ViewControllerBase

+(MainViewController*)instance;

@property(readonly, nonatomic) Renderer* renderer;
@property(readonly, nonatomic) World& world;

-(CameraInteraction*) interaction;
-(void)setInteraction:(CameraInteraction::Ptr)interaction;

@end
