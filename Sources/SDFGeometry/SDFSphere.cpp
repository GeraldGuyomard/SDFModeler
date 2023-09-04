//
//  SDFGeometry.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "SDFGeometry/SDFSphere.h"
#include "Type.h"

template <>
void initializeType<SDFSphere>(Type& type)
{
    type.addProperty<SDFSphere, float, &SDFSphere::radius, &SDFSphere::setRadius>("radius");
}

void
SDFSphere::setRadius(float radius)
{
    _radiusAndPadding.x = radius;
}
