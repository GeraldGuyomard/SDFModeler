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
#import "Animation.h"

@interface MainViewController : ViewControllerBase

@property(readonly, nonatomic) CGFloat nativeContentScale;
@property(readonly, nonatomic) Renderer* renderer;
@property(readonly, nonatomic) WorldPtr world;

-(Interaction::Ptr) interaction;
-(void)setInteraction:(Interaction::Ptr)interaction;

- (IBAction)undo:(id)source;
- (IBAction)redo:(id)source;
- (IBAction)delete:(id)source;
- (IBAction)group:(id)source;

- (void)frameAtPosition:(float2)pos wholeComposite:(BOOL)wholeComposite;

- (void)addAnimation:(Animation::Ptr)animation;
- (void)removeAnimation:(Animation::Ptr)animation;

- (Animation::Ptr) cameraAnimation;
- (void)setCameraAnimation:(Animation::Ptr)animation;

- (void) onSelectionChange;

@end
