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
    
    UIPanGestureRecognizer* orbitRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onOrbit:)];
    orbitRecognizer.minimumNumberOfTouches = 1;
    orbitRecognizer.maximumNumberOfTouches = 1;
    [self.view addGestureRecognizer:orbitRecognizer];

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
        
        if (auto object = self.world.objectByID(id))
        {
            object->setSelected(!object->selected());
        }
    }
}

- (void)onOrbit:(UIPanGestureRecognizer*)recognizer
{
    auto camera = self.renderer->camera();
    
    switch (recognizer.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
            auto camController = std::make_unique<OrbitCameraController>(camera, p, 2e-3f);
            [self setCameraController:std::move(camController)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto orbitController = dynamic_cast<OrbitCameraController*>(self.cameraController))
            {
                const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
                orbitController->orbit(p);
            }
            break;
        }
            
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
        {
            [self setCameraController:nullptr];
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
            auto camController = std::make_unique<PanCameraController>(camera, p);
            [self setCameraController:std::move(camController)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto panController = dynamic_cast<PanCameraController*>(self.cameraController))
            {
                const float2 p = [self convertPointToPixel:[recognizer locationInView:self.view]];
                panController->pan(p);
            }
            break;
        }
            
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
        {
            [self setCameraController:nullptr];
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
            auto camController = std::make_unique<DollyCameraController>(camera);
            [self setCameraController:std::move(camController)];
            break;
        }
            
        case UIGestureRecognizerStateChanged:
        {
            if (auto dollyController = dynamic_cast<DollyCameraController*>(self.cameraController))
            {
                const float d = -recognizer.velocity * 0.02f;
                dollyController->dolly(d);
            }
            break;
        }
            
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
        {
            [self setCameraController:nullptr];
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
