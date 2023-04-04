//
//  MainViewController+macOS.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerMacOS.h"
#include "World.h"

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
    
    CameraController::Ptr cameraController;
    const bool shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    if (shift)
    {
        cameraController = std::make_unique<PanCameraController>(camera, initialPos);
    }
    else
    {
        cameraController = std::make_unique<OrbitCameraController>(camera, initialPos);
    }
    
    [self setCameraController:std::move(cameraController)];
}

- (void)mouseDragged:(NSEvent *)event
{
    const auto pos = [self position:event.locationInWindow];
    
    auto cameraController = self.cameraController;
    if (auto orbit = dynamic_cast<OrbitCameraController*>(cameraController))
    {
        orbit->orbit(pos);
    }
    else if (auto pan = dynamic_cast<PanCameraController*>(cameraController))
    {
        pan->pan(pos);
    }
}

- (void)mouseUp:(NSEvent *)event
{
    [self setCameraController:nullptr];
}

- (void)scrollWheel:(NSEvent*)event
{
    float d = event.scrollingDeltaY / 1000.f;
    
    auto camController = std::make_unique<DollyCameraController>(self.renderer->camera());
    camController->dolly(d);
    [self setCameraController:std::move(camController)];
}

@end
