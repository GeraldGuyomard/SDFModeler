//
//  MainViewController+macOS.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#import <AppKit/AppKit.h>
#import "MainViewController.h"

@interface MainViewControllerMacOS : MainViewController

// rendering
- (IBAction)selectPhongRendering:(NSMenuItem*)source;
- (IBAction)selectCellShadedRendering:(NSMenuItem*)source;
- (IBAction)selectFlatRendering:(NSMenuItem*)source;

@property(nonatomic) IBOutlet NSMenuItem* phongMenuItem;
@property(nonatomic) IBOutlet NSMenuItem* cellShadedMenuItem;
@property(nonatomic) IBOutlet NSMenuItem* flatMenuItem;

@end
