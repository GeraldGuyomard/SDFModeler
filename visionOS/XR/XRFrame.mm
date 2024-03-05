//
//  RenderStats.cpp
//  SDFModeler
//
//  Created by Gerald Guyomard on 1/29/24.
//

#include "XRFrame.h"
#import <ARKit/ARKit.h>
#import <CompositorServices/CompositorServices.h>

#import "SDFModeler_visionOS-Swift.h"

XRFrame::XRFrame(XRFrameImpl* impl)
: _impl(impl)
{}

XRFrame::~XRFrame() = default;

void
XRFrame::startUpdate()
{
    [_impl startUpdate];
}

void
XRFrame::endUpdate()
{
    [_impl endUpdate];
}

bool
XRFrame::waitUntilOptimalTime()
{
    return [_impl waitUntilOptimalTime];
}

void
XRFrame::startSubmission()
{
    [_impl startSubmission];
}

void
XRFrame::endSubmission()
{
    [_impl endSubmission];
}
