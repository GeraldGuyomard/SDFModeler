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
    NSPoint ptInPixels = [self.view convertPoint:event.locationInWindow fromView:nil];
    
    NSRect r = self.view.frame;
    float2 size { float(r.size.width), float(r.size.height) };
    float2 p { float(ptInPixels.x), float(ptInPixels.y) };
    
    p = pixelToNDC(size, p);
    
    auto renderer = self.renderer;
    
    const auto& uniforms = renderer->uniforms();
    const auto& serializedWorld = renderer->serializedWorld();
    
    const auto pixel = renderDefault(p, uniforms, serializedWorld);
    
    NSLog(@"pixel R=%2.2f G=%2.2f B=%2.2f A=%2.2f\n", pixel.r, pixel.g, pixel.b, pixel.a);
}

- (CGPoint) position:(CGPoint)position
{
    position.y = self.view.frame.size.height - position.y;
    return position;
}

- (void)mouseDown:(NSEvent *)event
{
    auto camera = self.renderer->camera();
    
    const auto locInWindow = [self position:event.locationInWindow];
    const float2 initialPos { float(locInWindow.x), float(locInWindow.y) };
    
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
    auto locInWindow = [self position:event.locationInWindow];
    const float2 pos { float(locInWindow.x), float(locInWindow.y) };
    
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
