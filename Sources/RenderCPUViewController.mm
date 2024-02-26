//
//  RenderCPUViewController.m
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/4/23.
//

#import "RenderCPUViewController.h"
#include <vector>

#import "Renderer.h"
#include "CommonDefinitions.h"

#include "RenderFunctions.h"

#include "MainViewController.h"

@implementation RenderCPUViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
}

- (IBAction) render:(id)sender
{
    auto renderer = self.renderer;
    
    if (renderer == nullptr)
    {
        return;
    }
    
    auto img = renderer->renderImage();
    const CGSize size = img.size;
    
    [self.renderView setImage:img];
    
    // label
    NSString* title = [NSString stringWithFormat:@"Resolution %d x %d", int(size.width), int(size.height)];
    [self.resolutionLabel setStringValue:title];
}

@end
