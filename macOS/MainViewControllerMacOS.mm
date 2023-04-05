//
//  MainViewController+macOS.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerMacOS.h"
#include "World.h"
#include "CameraInteraction.h"

@implementation MainViewControllerMacOS

- (void)rightMouseDown:(NSEvent *)event
{
    const auto p = [self position:[self.view convertPoint:event.locationInWindow fromView:nil]];
    
    const auto id = self.renderer->pickObject(p);
    const auto pixel = self.renderer->renderPixel(p);
    
    NSLog(@"ObjectID = %d\n R=%1.4f G=%1.4f B=%1.4f A=%1.4f", id, pixel.x, pixel.y, pixel.z, pixel.w);
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
    const bool shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    if (shift)
    {
        interaction = std::make_unique<PanCameraInteraction>(camera, initialPos);
    }
    else
    {
        interaction = std::make_unique<OrbitCameraInteraction>(camera, initialPos);
    }
    
    [self setInteraction:std::move(interaction)];
}

- (void)mouseDragged:(NSEvent *)event
{
    const auto pos = [self position:event.locationInWindow];
    
    auto interaction = self.interaction;
    if (auto panInteraction = dynamic_cast<PanInteraction*>(interaction))
    {
        panInteraction->pan(pos);
    }
}

- (void)mouseUp:(NSEvent *)event
{
    [self setInteraction:nullptr];
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto interaction = std::make_unique<DollyCameraInteraction>(self.renderer->camera());
    interaction->pinch(d);
    [self setInteraction:std::move(interaction)];
}

@end
