//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "SDFGeometry/SDFCone.h"
#include "Type.h"

void
SDFCone::setRadius(float r)
{
    _params.x = r;
}

void
SDFCone::setHeight(float h)
{
    _params.y = h;
}

template <>
void initializeType<SDFCone>(Type& type)
{
    type.addProperty<SDFCone, float, &SDFCone::radius, &SDFCone::setRadius>("radius");
    type.addProperty<SDFCone, float, &SDFCone::height, &SDFCone::setHeight>("height");
}
