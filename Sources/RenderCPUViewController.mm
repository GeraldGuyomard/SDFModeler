//
//  RenderCPUViewController.m
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/4/23.
//

#import "RenderCPUViewController.h"
#include <vector>

#import "Renderer.h"
#include "CommonDefinitions.h"

#include "World.h"

#include "MainViewController.h"

@implementation RenderCPUViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
}

- (IBAction) render:(id)sender
{
    //CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
    auto rgbColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear);
    
    auto gameViewController = [MainViewController instance];
    auto renderer = gameViewController.renderer;
    
    const auto size = renderer->renderSize();
    
    std::vector<uint32_t> buffer;
    buffer.resize(size.x * size.y);
    
    const auto& uniforms = renderer->uniforms();
    const auto& serializedWorld = renderer->serializedWorld();
    
    for (float y=0; y < size.y; ++y)
    {
        for (float x=0; x < size.x; ++x)
        {
            uint32_t& pixel = buffer[(y * size.x) + x];
            const float2 pixelCoordinates { x, y };
            
            const auto p = pixelToNDC(size, pixelCoordinates);
            
            const auto fragment = renderDefault(p, uniforms, serializedWorld);
            const uint8_t r = clamp(fragment.r, 0.f, 1.f) * 255.f;
            const uint8_t g = clamp(fragment.g, 0.f, 1.f) * 255.f;
            const uint8_t b = clamp(fragment.b, 0.f, 1.f) * 255.f;
            
            uint32_t v = r | (g << 8) | (b << 16) | 0xFF000000;
            pixel = v;
        }
    }
    
    /*for (uint32_t& pixel : buffer)
    {
        // ABGR
        pixel = 0x4000FF00;
    }*/
    
    const size_t bytesPerRow = size.x * sizeof(uint32_t);
    CGContextRef context = CGBitmapContextCreateWithData(buffer.data(),
                                  size.x,
                                  size.y,
                                  8,
                                  bytesPerRow,
                                  rgbColorSpace,
                                    kCGImageAlphaPremultipliedLast,
                                  nullptr,
                                  nullptr
                                  );
    
    CGColorSpaceRelease(rgbColorSpace);

    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    
    NSImage* img = [[NSImage alloc] initWithCGImage:cgImage size:CGSizeMake(size.x, size.y)];
    CGImageRelease(cgImage);
    
    [self.renderView setImage:img];
    
    // label
    NSString* title = [NSString stringWithFormat:@"Resolution %d x %d", int(size.x), int(size.y)];
    [self.resolutionLabel setStringValue:title];
}

@end
