//
//  MainViewControllerIOS.cpp
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 4/1/23.
//

#include "MainViewControllerIOS.h"
#include <chrono>

using HighResClock = std::chrono::high_resolution_clock;

@implementation MainViewControllerIOS
{
    HighResClock::time_point _lastRenderTime;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UITapGestureRecognizer* tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
    tapRecognizer.numberOfTapsRequired = 1;
    [self.view addGestureRecognizer:tapRecognizer];
    
    UIPanGestureRecognizer* panOneFingerRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanOneFinger:)];
    panOneFingerRecognizer.minimumNumberOfTouches = 1;
    panOneFingerRecognizer.maximumNumberOfTouches = 1;
    [self.view addGestureRecognizer:panOneFingerRecognizer];

    UIPanGestureRecognizer* panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPan:)];
    panRecognizer.minimumNumberOfTouches = 2;
    [self.view addGestureRecognizer:panRecognizer];
    
    UIPinchGestureRecognizer* pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(onPinch:)];
    [self.view addGestureRecognizer:pinchRecognizer];
    
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

- (float2) convertPointToPixel:(CGPoint)pt
{
    UIView* view = self.view;
    CAMetalLayer* metalLayer = (CAMetalLayer*) view.layer;
    
    const float contentScale = metalLayer.contentsScale;
    
    return { float(pt.x) * contentScale, float(pt.y) * contentScale };
}

- (void)onTap:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateEnded)
    {
        const auto p = [self convertPointToPixel:[recognizer locationInView:self.view]];
        
        const auto id = self.renderer->pickObject(p);
        NSLog(@"ObjectID = %d\n", id);
        
        Selection selection;
        if (auto object = self.world.objectByID(id))
        {
            selection.addObject(object);
        }
        
        self.world.setSelection(selection);
    }
}

- (void)onPanOneFinger:(UIPanGestureRecognizer*)recognizer
{
    auto camera = self.renderer->camera();
    
    switch (recognizer.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
            
            auto interaction = std::make_unique<OrbitCameraInteraction>(camera, p, 2e-3f);
            [self setInteraction:std::move(interaction)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto orbitInteraction = dynamic_cast<OrbitCameraInteraction*>(self.interaction))
            {
                const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
                orbitInteraction->orbit(p);
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

- (void)onPan:(UIPanGestureRecognizer*)recognizer
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
            if (auto panInteraction = dynamic_cast<PanCameraInteraction*>(self.interaction))
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
            auto interaction = std::make_unique<DollyCameraInteraction>(camera);
            [self setInteraction:std::move(interaction)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto dollyInteraction = dynamic_cast<DollyCameraInteraction*>(self.interaction))
            {
                const float d = -recognizer.velocity * 0.02f;
                dollyInteraction->dolly(d);
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
