//
//  MainViewControllerIOS.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerIOS.h"

#include "CameraInteraction.h"
#include "Object3DInteraction.h"
#include "MultiTouchCameraInteraction.h"

#include "AddObjectCommand.h"
#include "RemoveObjectCommand.h"
#include "ToggleObjectOperationCommand.h"

@interface MainViewControllerIOS()<UIGestureRecognizerDelegate>
@end

@implementation MainViewControllerIOS
{
    UITapGestureRecognizer* _singleTapRecognizer;
    UITapGestureRecognizer* _doubleTapRecognizer;
    UILongPressGestureRecognizer* _longPressRecognizer;
    
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

    _doubleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onDoubleTap:)];
    _doubleTapRecognizer.numberOfTapsRequired = 2;
    _doubleTapRecognizer.numberOfTouchesRequired = 1;
    
    _longPressRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onLongPress:)];
    
    _dragObjectRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onDragObject:)];
    _dragObjectRecognizer.minimumNumberOfTouches = 1;
    _dragObjectRecognizer.maximumNumberOfTouches = 1;
    
    _undoRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTapTwoFingers:)];
    _undoRecognizer.numberOfTapsRequired = 1;
    _undoRecognizer.numberOfTouchesRequired = 2;
    
    _redoRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTapThreeFingers:)];
    _redoRecognizer.numberOfTapsRequired = 1;
    _redoRecognizer.numberOfTouchesRequired = 3;
    
    self.view.gestureRecognizers = @[_singleTapRecognizer, _doubleTapRecognizer, _longPressRecognizer, _dragObjectRecognizer, _undoRecognizer, _redoRecognizer];
    
    for (UIGestureRecognizer* recognizer in self.view.gestureRecognizers)
    {
        recognizer.delegate = self;
    }
    
    [self updateUndoRedoButtons];
    
    __weak auto wSelf = self;
    self.world->commandHistory().setStateUpdateCallback([wSelf](const auto& history)
    {
        if (auto self = wSelf)
        {
            [self updateUndoRedoButtons];
        }
    });
    
    self.selectionActionsButton.showsMenuAsPrimaryAction = YES;
    [self updateActionsButton];
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)recognizer
{
    if (recognizer == _dragObjectRecognizer)
    {
        auto currentCameraInteraction = std::dynamic_pointer_cast<MultiTouchCameraInteraction>(self.interaction);
        if ((currentCameraInteraction != nullptr) && (currentCameraInteraction->state() == MultiTouchCameraInteraction::State::active))
        {
            return NO;
        }
        
        auto info = [self objectDragInfo:[recognizer locationInView:self.view]];
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
        const auto selectedObject = self.world->selectedObject();
        
        MultiTouchCameraInteraction::Ptr interaction = std::make_shared<MultiTouchCameraInteraction>(camera, self.renderer);

        interaction->setOrbitSpeed(MultiTouchCameraInteraction::kDefaultOrbitSpeed * self.contentScale);
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
            auto anim = cameraInteraction->makeOrbitDecelerationAnimation();
            [self setInteraction:nil];
            
            [self setCameraAnimation:anim];
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    [self touchesEnded:touches withEvent:event];
}

- (Object3D::Ptr)objectFromPosition:(float2)pt
{
    const auto pickResult = self.renderer->pick(pt);
    NSLog(@"ObjectID = %d\n", pickResult.objectID);
    
    return self.world->rootObject()->objectByID(pickResult.objectID);
}

- (void)selectObjectAtPosition:(float2)pt
{
    auto object = [self objectFromPosition:pt];
    self.world->setSelectedObject(object);
    
    auto camera = self.renderer->camera();
    camera->setLookAtPositionProvider(object);
    
    [self updateActionsButton];
}

- (void)onTap:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        const auto p = [self convertPointToPixel:[recognizer locationInView:self.view]];
        [self selectObjectAtPosition:p];
    }
}

- (void)onLongPress:(UILongPressGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateBegan)
    {
        const auto p = [self convertPointToPixel:[recognizer locationInView:self.view]];
        [self selectObjectAtPosition:p];
    }
}

- (void)onDoubleTap:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        const auto p = [self convertPointToPixel:[recognizer locationInView:self.view]];
        [self frameAtPosition:p];
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

- (ObjectDragInfo::Ptr) objectDragInfo:(CGPoint)location
{
    const float2 p = [self convertPointToPixel:location];
    
    const auto pickResult = self.renderer->pick(p);
    
    if (auto object = self.world->rootObject()->objectByID(pickResult.objectID))
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
            if (auto info = [self objectDragInfo:[recognizer locationInView:self.view]])
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

- (float) contentScale
{
    return self.view.layer.contentsScale;
}

- (void) adjustContentScale
{
    //const CGFloat ratio = (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad) ? 1.f : 1.5f;
  
    UIView* view = self.view;
    CAMetalLayer* metalLayer = (CAMetalLayer *)view.layer;
    
    const CGSize size = self.view.bounds.size;
    
    const CGFloat contentScaleFactor = self.nativeContentScale * 0.75f;
  
    metalLayer.contentsScale = contentScaleFactor;
    
    CGSize drawableSize;
    drawableSize.width = size.width * contentScaleFactor;
    drawableSize.height = size.height * contentScaleFactor;
    
    metalLayer.drawableSize = drawableSize;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    [self adjustContentScale];
}

- (void) viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    [self adjustContentScale];
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
    const auto& commandHistory = self.world->commandHistory();
    
    enableButton(self.undoButton, commandHistory.canUndo());
    enableButton(self.redoButton, commandHistory.canRedo());
}

- (UIMenu*)makeAddObjectMenu
{
    NSMutableArray<UIMenuElement*>* children = [NSMutableArray new];
    
    for (const auto& factory : Object3DFactory::factories())
    {
        NSString* title = [NSString stringWithUTF8String:factory->name().c_str()];
        auto action = [UIAction actionWithTitle:title
                                image:nil
                                identifier:title
                                handler:^(UIAction * action)
        {
            
            auto world = self.world;
            
            auto command = std::make_shared<AddObjectCommand>(world->rootObject(), factory);
            world->commandHistory().run(command);
            
        }];
        
        [children addObject:action];
        
    }
    
    UIMenu* menu = [UIMenu menuWithTitle:@"Add Object..."
                    image:nil
                    identifier:@"AddObjectMenu"
                    options:0
                    children:children];
    
    return menu;
}

- (void) updateActionsButton
{
    NSMutableArray<UIMenuElement*>* children = [NSMutableArray new];
    
    auto world = self.world;
    auto selectedObject = world->selectedObject();
    if (selectedObject != nullptr)
    {
        auto removeObjectCommand = std::make_shared<RemoveObjectCommand>(selectedObject);
        
        auto deleteAction = [UIAction actionWithTitle:@"Delete"
                                                image:nil
                                           identifier:@"Delete"
                                              handler:^(UIAction * action)
                             {
            world->commandHistory().run(removeObjectCommand);
        }];
        
        [children addObject:deleteAction];
        
        auto toggleObjectOperationCommand = std::make_shared<ToggleObjectOperationCommand>(selectedObject);
        
        auto toggleAction = [UIAction actionWithTitle:@"Toggle Operation"
                                                image:nil
                                           identifier:@"Toggle"
                                              handler:^(UIAction * action)
                             {
            world->commandHistory().run(toggleObjectOperationCommand);
        }];
        
        [children addObject:toggleAction];
    }
        
    [children addObject:[self makeAddObjectMenu]];
    
    auto menu = [UIMenu menuWithTitle:@"Edit"
                    image:nil
                    identifier:@"Edit"
                    options:0
                    children:children];
    
    self.selectionActionsButton.menu = menu;
    
    [self.selectionActionsButton sizeToFit];
}

@end
