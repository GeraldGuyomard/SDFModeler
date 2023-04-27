//
//  RenderCPUViewController.m
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/4/23.
//

#import <Cocoa/Cocoa.h>

class Renderer;

// Our macOS view controller.
@interface RenderCPUViewController : NSViewController

- (IBAction) render:(id)sender;

@property(nonatomic) IBOutlet NSImageView* renderView;
@property(nonatomic) IBOutlet NSTextField* resolutionLabel;

@property(nonatomic) Renderer* renderer;

@end
