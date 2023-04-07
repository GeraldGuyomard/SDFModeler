//
//  MainViewControllerIOS.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerIOS.h"
#include <chrono>
#include "CameraInteraction.h"
#include "Object3DInteraction.h"

using HighResClock = std::chrono::high_resolution_clock;

@interface MainViewControllerIOS()<UIGestureRecognizerDelegate>
@end

@implementation MainViewControllerIOS
{
    HighResClock::time_point _lastRenderTime;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UITapGestureRecognizer* singleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
    singleTapRecognizer.numberOfTapsRequired = 1;
    
    UIPanGestureRecognizer* panOneFingerRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanOneFinger:)];
    panOneFingerRecognizer.minimumNumberOfTouches = 1;
    panOneFingerRecognizer.maximumNumberOfTouches = 1;
    
    UIPanGestureRecognizer* panTwoFingersRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanTwoFingers:)];
    panTwoFingersRecognizer.minimumNumberOfTouches = 2;
    
    UIPinchGestureRecognizer* pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(onPinch:)];
    
    auto recognizers = @[singleTapRecognizer, panOneFingerRecognizer, panTwoFingersRecognizer, pinchRecognizer];
    for (UIGestureRecognizer* recognizer in recognizers)
    {
        recognizer.delegate = self;
    }
    
    self.view.gestureRecognizers = recognizers;
    
    _lastRenderTime = HighResClock::now();
    
    __weak auto wSelf = self;
    self.renderer->setRenderCallback([wSelf](auto& renderer)
    {
        if (auto self = wSelf)
        {
            auto now = HighResClock::now();
            const auto dT = now - _lastRenderTime;
            _lastRenderTime = now;
            
            const float millis = std::chrono::duration_cast<std::chrono::milliseconds>(dT).count();
            const float fps = 1000.f / millis;
            
            self.fpsLabel.text = [NSString stringWithFormat:@"FPS = %2.2f", fps];
            [self.fpsLabel sizeToFit];
        }
    });
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return NO;
}

- (float2) convertPointToPixel:(CGPoint)pt
{
    UIView* view = self.view;
    CAMetalLayer* metalLayer = (CAMetalLayer*) view.layer;
    
    const float contentScale = metalLayer.contentsScale;
    
    return { float(pt.x) * contentScale, float(pt.y) * contentScale };
}

- (Selection)selectionFromPosition:(float2)pt
{
    const auto pickResult = self.renderer->pick(pt);
    NSLog(@"ObjectID = %d\n", pickResult.objectID);
    
    Selection selection;
    if (auto object = self.world.objectByID(pickResult.objectID))
    {
        selection.addObject(object);
    }
    
    return selection;
}

- (void)selectObjectAtPosition:(float2)pt
{
    self.world.setSelection([self selectionFromPosition:pt]);
}

- (void)onTap:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        const auto p = [self convertPointToPixel:[recognizer locationInView:self.view]];
        [self selectObjectAtPosition:p];
    }
}

- (void)onPanOneFinger:(UIPanGestureRecognizer*)recognizer
{
    auto camera = self.renderer->camera();
    
    switch (recognizer.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            Interaction::Ptr interaction;
            
            const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
            
            const auto pickResult = self.renderer->pick(p);
            
            if (auto object = self.world.objectByID(pickResult.objectID))
            {
                const Selection selection = self.world.selection();
                if (selection.contains(object))
                {
                    interaction = std::make_unique<DragObject3DInteraction>(object, pickResult.position, p, *self.renderer);
                }
            }
            
            if (interaction == nullptr)
            {
                constexpr float speed = 2e-3f;
                
                if (auto selectedObject = self.world.selection().anyObject())
                {
                    const float3 origin = translation(selectedObject->worldTransform());
                    interaction = std::make_unique<OrbitCameraInteraction>(camera, origin, p, speed);
                }
                else
                {
                    interaction = std::make_unique<OrbitCameraInteraction>(camera, p, speed);
                }
            }
            
            [self setInteraction:std::move(interaction)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto panInteraction = dynamic_cast<PanInteraction*>(self.interaction))
            {
                const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
                panInteraction->pan(p);
            }
            break;
        }
            
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
        {
            [self setInteraction:nullptr];
            break;
        }
            
        default: break;
    }
}

- (void)onPanTwoFingers:(UIPanGestureRecognizer*)recognizer
{
    auto camera = self.renderer->camera();
    
    switch (recognizer.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
            auto interaction = std::make_unique<PanCameraInteraction>(camera, p);
            [self setInteraction:std::move(interaction)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto panInteraction = dynamic_cast<PanInteraction*>(self.interaction))
            {
                const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
                panInteraction->pan(p);
            }
            break;
        }
            
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
        {
            [self setInteraction:nullptr];
            break;
        }
            
        default: break;
    }
}


- (void)onPinch:(UIPinchGestureRecognizer*)recognizer
{
    auto camera = self.renderer->camera();
    
    switch (recognizer.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            DollyCameraInteraction::Ptr interaction;
            
            if (auto object = self.world.selection().anyObject())
            {
                interaction = std::make_unique<DollyCameraInteraction>(camera, object);
            }
            else
            {
                interaction = std::make_unique<DollyCameraInteraction>(camera);
            }
            
            [self setInteraction:std::move(interaction)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto pinchInteraction = dynamic_cast<PinchInteraction*>(self.interaction))
            {
                const float d = -recognizer.velocity * 0.02f;
                pinchInteraction->pinch(d);
            }
            break;
        }
            
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
        {
            [self setInteraction:nullptr];
            break;
        }
            
        default: break;
    }
}

- (void) setContentScaleFactor:(CGFloat)sf size:(CGSize)size
{
    UIView* view = self.view;
    CAMetalLayer* metalLayer = (CAMetalLayer *)view.layer;
    
    metalLayer.contentsScale = sf;
    
    CGSize drawableSize;
    drawableSize.width = size.width * sf;
    drawableSize.height = size.height * sf;
    
    metalLayer.drawableSize = drawableSize;
}

static constexpr CGFloat kContentScaleFactor = 2.f;

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    const CGSize size = self.view.bounds.size;
    [self setContentScaleFactor:kContentScaleFactor size:size];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    [self setContentScaleFactor:kContentScaleFactor size:size];
}


@end
