//
//  MainViewController+macOS.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerMacOS.h"
#include "RenderFunctions.h"
#include "CameraInteraction.h"
#include "Object3DInteraction.h"

@interface MainViewControllerMacOS()<NSMenuItemValidation>
@end

@implementation MainViewControllerMacOS

- (void)viewDidAppear
{
    [super viewDidAppear];
 
    [self.view.window makeFirstResponder:self];
}

- (void)rightMouseDown:(NSEvent *)event
{
    const auto p = [self position:[self.view convertPoint:event.locationInWindow fromView:nil]];
    
    const auto res = self.renderer->pick(p);
    const auto pixel = self.renderer->renderPixel(p);
    
    NSLog(@"ObjectID = %d\n R=%1.4f G=%1.4f B=%1.4f A=%1.4f", res.objectID, pixel.x, pixel.y, pixel.z, pixel.w);
}

- (float2) position:(CGPoint)position
{
    CGPoint physPt = [self.view convertPointToBacking:position];
    const CGRect physRect = [self.view convertRectToBacking:self.view.frame];
    
    return { float(physPt.x), float(physRect.size.height - physPt.y) };
}

- (void)mouseDown:(NSEvent *)event
{
    auto camera = self.renderer->camera();
    
    const auto initialPos = [self position:event.locationInWindow];
    
    Interaction::Ptr interaction;
    
    auto world = self.world;
    
    const auto result = self.renderer->pick(initialPos);
    if (auto object = world->rootObject()->objectByID(result.objectID))
    {
        if ((event.modifierFlags & NSEventModifierFlagOption) == 0)
        {
            object = object->owner();
        }
        
        Object3DSelection sel;
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0)
        {
            sel = world->selection();
            sel.add(object);
        }
        else
        {
            sel = { object };
        }
        
        world->setSelection(sel);
        camera->setLookAtPositionProvider(object);
        
        interaction = std::make_shared<DragObject3DInteraction>(world,
                                                                world->selection(),
                                                                result.position,
                                                                initialPos,
                                                                *self.renderer);
    }
    else
    {
        // camera
        const bool shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
        if (shift)
        {
            interaction = std::make_shared<PanCameraInteraction>(camera, initialPos);
        }
        else
        {
            interaction = std::make_shared<OrbitCameraInteraction>(camera, initialPos);
        }
    }
    
    [self setInteraction:std::move(interaction)];
}

- (void)mouseDragged:(NSEvent *)event
{
    const auto pos = [self position:event.locationInWindow];
    
    auto interaction = self.interaction;
    if (auto panInteraction = std::dynamic_pointer_cast<PanInteraction>(interaction))
    {
        panInteraction->pan(pos);
    }
}

- (void)mouseUp:(NSEvent *)event
{
    [self setInteraction:nullptr];
    
    if (event.clickCount == 2)
    {
        const auto pos = [self position:event.locationInWindow];
        const bool optionDown = (event.modifierFlags & NSEventModifierFlagOption) != 0;
        [self frameAtPosition:pos owner:!optionDown];
    }
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto camera = self.renderer->camera();
    
    auto interaction = std::make_unique<DollyCameraInteraction>(camera);
    interaction->pinch(d);
    
    [self setInteraction:std::move(interaction)];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)resignFirstResponder
{
    return NO;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (menuItem.action == @selector(undo:))
    {
        return self.world->commandHistory().canUndo();
    }
    else if (menuItem.action == @selector(redo:))
    {
        return self.world->commandHistory().canRedo();
    }
    else if (menuItem.action == @selector(delete:))
    {
        return !self.world->selection().empty();
    }
    else if (menuItem.action == @selector(group:))
    {
        return !self.world->selection().empty();
    }
    else if (menuItem.action == @selector(toggleOperation:))
    {
        return !self.world->selection().empty();
    }
    
    return NO;
}

@end
