//
//  AppDelegate.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "AppDelegate.h"
#import "RenderCPUViewController.h"
#import "MainViewController.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)prepareForSegue:(NSStoryboardSegue *)segue sender:(nullable id)sender
{
    NSWindowController* controller = segue.destinationController;
    NSViewController* viewController = controller.contentViewController;
    if ([viewController isKindOfClass:RenderCPUViewController.class])
    {
        NSWindow* mainWindow = [NSApplication sharedApplication].mainWindow;
        MainViewController* mainViewController = (MainViewController*) mainWindow.windowController.contentViewController;
        
        if (mainViewController != nil)
        {
            RenderCPUViewController* renderController = (RenderCPUViewController*)viewController;
            renderController.renderer = mainViewController.renderer;
        }
    }
}

@end
