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
    
    UITapGestureRecognizer* _singleTapRecognizer;
    UIPanGestureRecognizer* _dragObjectRecognizer;
    UITapGestureRecognizer* _undoRecognizer;
    UITapGestureRecognizer* _redoRecognizer;
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
    
    _singleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
    _singleTapRecognizer.numberOfTapsRequired = 1;
    _singleTapRecognizer.numberOfTouchesRequired = 1;
    
    _dragObjectRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onDragObject:)];
    _dragObjectRecognizer.minimumNumberOfTouches = 1;
    _dragObjectRecognizer.maximumNumberOfTouches = 1;
    
    _undoRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTapTwoFingers:)];
    _undoRecognizer.numberOfTapsRequired = 1;
    _undoRecognizer.numberOfTouchesRequired = 2;
    
    _redoRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTapThreeFingers:)];
    _redoRecognizer.numberOfTapsRequired = 1;
    _redoRecognizer.numberOfTouchesRequired = 3;
    
    self.view.gestureRecognizers = @[_singleTapRecognizer, _dragObjectRecognizer, _undoRecognizer, _redoRecognizer];
    
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
    
    [self updateUndoRedoButtons];
    
    self.world.commandHistory().setStateUpdateCallback([wSelf](const auto& history)
    {
        if (auto self = wSelf)
        {
            [self updateUndoRedoButtons];
        }
    });
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer
{
    if (gestureRecognizer == _dragObjectRecognizer)
    {
        auto currentCameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction);
        if ((currentCameraInteraction != nullptr) && (currentCameraInteraction->state() == MultiTouchCameraInteraction::State::active))
        {
            return NO;
        }
        
        auto info = [self objectDragInfo:gestureRecognizer];
        return info != nullptr;
    }
    
    return YES;
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
        
        const auto selectedObject = self.world.selectedObject();
        
        Interaction::Ptr interaction;
        auto object = self.world.objectByID(pickResult.objectID);
        
        if (selectedObject != nullptr)
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

- (Object3D::Ptr)objectFromPosition:(float2)pt
{
    const auto pickResult = self.renderer->pick(pt);
    NSLog(@"ObjectID = %d\n", pickResult.objectID);
    
    return self.world.objectByID(pickResult.objectID);
}

- (void)selectObjectAtPosition:(float2)pt
{
    self.world.setSelectedObject([self objectFromPosition:pt]);
}

- (void)onTap:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        const auto p = [self convertPointToPixel:[recognizer locationInView:self.view]];
        [self selectObjectAtPosition:p];
    }
}

struct ObjectDragInfo
{
    using Ptr = std::unique_ptr<ObjectDragInfo>;
    
    const Object3D::Ptr object;
    const float2 positionInNDC;
    const float3 position3D;
    
    ObjectDragInfo(const Object3D::Ptr& object, const float2& positionInNDC, const float3& position3D)
    : object(object), positionInNDC(positionInNDC), position3D(position3D)
    {}
};

- (ObjectDragInfo::Ptr) objectDragInfo:(UIGestureRecognizer*)recognizer
{
    const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
    
    const auto pickResult = self.renderer->pick(p);
    
    if (auto object = self.world.objectByID(pickResult.objectID))
    {
        if (object->selected())
        {
            return std::make_unique<ObjectDragInfo>(object, p, pickResult.position);
        }
    }
    
    return nullptr;
}

- (void)onDragObject:(UIPanGestureRecognizer*)recognizer
{
    switch (recognizer.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            if (auto info = [self objectDragInfo:recognizer])
            {
                auto interaction = std::make_shared<DragObject3DInteraction>(self.world,
                                                                             info->object,
                                                                             info->position3D,
                                                                             info->positionInNDC,
                                                                             *self.renderer);
                [self setInteraction:interaction];
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

- (void)onTapTwoFingers:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        [self undo:nil];
    }
}

- (void)onTapThreeFingers:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        [self redo:nil];
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

static constexpr CGFloat kContentScaleFactor = 1.f;

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    const CGSize size = self.view.bounds.size;
    //[self setContentScaleFactor:kContentScaleFactor size:size];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    //[self setContentScaleFactor:kContentScaleFactor size:size];
}

namespace
{
    void enableButton(UIButton* button, bool enable)
    {
        button.enabled = enable;
        button.alpha = enable ? 1.f : 0.5f;
    }
}

- (void)updateUndoRedoButtons
{
    const auto& commandHistory = self.world.commandHistory();
    
    enableButton(self.undoButton, commandHistory.canUndo());
    enableButton(self.redoButton, commandHistory.canRedo());
}

@end
