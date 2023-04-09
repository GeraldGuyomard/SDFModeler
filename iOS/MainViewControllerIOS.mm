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
#include "MultiTouchCameraInteraction.h"

using HighResClock = std::chrono::high_resolution_clock;

@interface MainViewControllerIOS()<UIGestureRecognizerDelegate>
@end

@implementation MainViewControllerIOS
{
    HighResClock::time_point _lastRenderTime;
}

- (float2) convertPointToPixel:(CGPoint)pt
{
    UIView* view = self.view;
    CAMetalLayer* metalLayer = (CAMetalLayer*) view.layer;
    
    const float contentScale = metalLayer.contentsScale;
    
    return { float(pt.x) * contentScale, float(pt.y) * contentScale };
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.view.multipleTouchEnabled = YES;
    
    UITapGestureRecognizer* singleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
    singleTapRecognizer.numberOfTapsRequired = 1;
    
    UIPanGestureRecognizer* panOneFingerRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanOneFinger:)];
    panOneFingerRecognizer.minimumNumberOfTouches = 1;
    panOneFingerRecognizer.maximumNumberOfTouches = 1;
    
    /*UIPanGestureRecognizer* panTwoFingersRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanTwoFingers:)];
    panTwoFingersRecognizer.minimumNumberOfTouches = 2;
    
    UIPinchGestureRecognizer* pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(onPinch:)];
    
    auto recognizers = @[singleTapRecognizer, panOneFingerRecognizer, panTwoFingersRecognizer, pinchRecognizer];
    for (UIGestureRecognizer* recognizer in recognizers)
    {
        recognizer.delegate = self;
    }
    
    self.view.gestureRecognizers = recognizers;*/
    
    self.view.gestureRecognizers = @[singleTapRecognizer, panOneFingerRecognizer];
    
    for (UIGestureRecognizer* recognizer in self.view.gestureRecognizers)
    {
        recognizer.delegate = self;
    }
    
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

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer
{
    if ([gestureRecognizer isKindOfClass:UIPanGestureRecognizer.class])
    {
        auto currentCameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction);
        return currentCameraInteraction == nullptr;
    }
    else
    {
        return YES;
    }
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return NO;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    auto interaction = self.interaction;
    
    if (interaction == nullptr)
    {
        auto camera = self.renderer->camera();
        
        UITouch* touch = [touches anyObject];
        
        const float2 p = [self convertPointToPixel:[touch locationInView:self.view]];
        
        const auto pickResult = self.renderer->pick(p);
        
        const Selection selection = self.world.selection();
        
        Interaction::Ptr interaction;
        auto object = self.world.objectByID(pickResult.objectID);
        
        if (object != nullptr)
        {
            // let the regular pan gesture run
        }
        else if (auto selectedObject = selection.anyObject())
        {
            const float3 origin = translation(selectedObject->worldTransform());
            interaction = std::make_shared<MultiTouchCameraInteraction>(camera, self.renderer, origin);
        }
        else
        {
            interaction = std::make_shared<MultiTouchCameraInteraction>(camera, self.renderer);
        }

        [self setInteraction:interaction];
    }
    
    if (auto cameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction))
    {
        cameraInteraction->touchesBegan(touches);
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    if (auto cameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction))
    {
        cameraInteraction->touchesMoved(touches);
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    if (auto cameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction))
    {
        cameraInteraction->touchesEnded(touches);
        if (cameraInteraction->state() == MultiTouchCameraInteraction::State::done)
        {
            [self setInteraction:nil];
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    if (auto cameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction))
    {
        cameraInteraction->touchesEnded(touches);
        if (cameraInteraction->state() == MultiTouchCameraInteraction::State::done)
        {
            [self setInteraction:nil];
        }
    }
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
                    interaction = std::make_shared<DragObject3DInteraction>(object, pickResult.position, p, *self.renderer);
                    [self setInteraction:interaction];
                }
            }
            
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto panInteraction = std::dynamic_pointer_cast<PanInteraction>(self.interaction))
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
