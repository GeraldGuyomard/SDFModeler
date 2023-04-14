//
//  MainViewControllerIOS.hpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#pragma once

#import "MainViewController.h"

@interface MainViewControllerIOS : MainViewController

@property(nonatomic) IBOutlet UIButton* selectionActionsButton;

@property(nonatomic) IBOutlet UIButton* undoButton;
@property(nonatomic) IBOutlet UIButton* redoButton;

@end
