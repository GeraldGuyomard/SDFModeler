//
//  SDFGeometry.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "SDFGeometry/SDFTorus.h"
#include "Type.h"

void
SDFTorus::setRadius(float r)
{
    _radiusThicknessAndPadding.x = r;
}

void
SDFTorus::setThickness(float t)
{
    _radiusThicknessAndPadding.y = t;
}

template <>
void initializeType<SDFTorus>(Type& type)
{
    type.addProperty<SDFTorus, float, &SDFTorus::radius, &SDFTorus::setRadius>("radius");
    type.addProperty<SDFTorus, float, &SDFTorus::thickness, &SDFTorus::setThickness>("thickness");
}
