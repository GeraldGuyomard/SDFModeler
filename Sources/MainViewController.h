//
//  MainViewController.h
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//


#import "CommonDefinitions.h"

#import "Renderer.h"
#import "Object3D.h"
#import "Interaction.h"

// Our macOS view controller.
@interface MainViewController : ViewControllerBase

+(MainViewController*)instance;

@property(readonly, nonatomic) CGFloat nativeContentScale;
@property(readonly, nonatomic) Renderer* renderer;
@property(readonly, nonatomic) WorldPtr world;

-(Interaction::Ptr) interaction;
-(void)setInteraction:(Interaction::Ptr)interaction;

- (IBAction)undo:(id)source;
- (IBAction)redo:(id)source;

- (void)frameAtPosition:(float2)pos;

@end
