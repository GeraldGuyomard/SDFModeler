//
//  SDFGeometry.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "SDFGeometry/SDFCylinder.h"
#include "Type.h"

void
SDFCylinder::setRadius(float r)
{
    _radiusHeightAndPadding.x = r;
}

void
SDFCylinder::setHeight(float h)
{
    _radiusHeightAndPadding.y = h;
}

template <>
void initializeType<SDFCylinder>(Type& type)
{
    type.addProperty<SDFCylinder, float, &SDFCylinder::radius, &SDFCylinder::setRadius>("radius");
    type.addProperty<SDFCylinder, float, &SDFCylinder::height, &SDFCylinder::setHeight>("height");
}
