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

#include "GameViewController.h"

@implementation RenderCPUViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
}

- (IBAction) render:(id)sender
{
    //CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
    auto rgbColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear);
    
    auto gameViewController = [GameViewController instance];
    auto renderer = gameViewController.renderer;
    
    const auto size = gameViewController.view.bounds.size;
    
    std::vector<uint32_t> buffer;
    buffer.resize(size.width * size.height);
    
    const float2 viewportSize { (float)size.width, (float)size.height };
    const PhongShader shader;
    const auto* uniforms = renderer.uniforms;
    
    for (float y=0; y < size.height; ++y)
    {
        for (float x=0; x < size.width; ++x)
        {
            uint32_t& pixel = buffer[(y * size.width) + x];
            const float2 pixelCoordinates { x, viewportSize.y - y - 1.f };
            
            const auto p = pixelToNDC(viewportSize, pixelCoordinates);
            
            const auto fragment = render(p, shader, *uniforms);
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
    
    const size_t bytesPerRow = size.width * sizeof(uint32_t);
    CGContextRef context = CGBitmapContextCreateWithData(buffer.data(),
                                  size.width,
                                  size.height,
                                  8,
                                  bytesPerRow,
                                  rgbColorSpace,
                                    kCGImageAlphaPremultipliedLast,
                                  nullptr,
                                  nullptr
                                  );
    
    CGColorSpaceRelease(rgbColorSpace);

    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    NSImage* img = [[NSImage alloc] initWithCGImage:cgImage size:size];
    CGImageRelease(cgImage);
    
    [self.renderView setImage:img];
}

@end
