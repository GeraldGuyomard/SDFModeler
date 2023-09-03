//
//  SDFGeometry.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "SDFGeometry/SDFSphere.h"
#include "Type.h"

class TaMere
{
public:
    float radius() const;
};

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
