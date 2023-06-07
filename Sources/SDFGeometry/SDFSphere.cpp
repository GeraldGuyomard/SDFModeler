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
    type.addProperty<SDFSphere, float>
    ("radius",
    [](const SDFSphere* object) -> float
    {
        return object->radius();
    },
                                           
    [](SDFSphere* object, const float& v)
    {
        object->setRadius(v);
    });
}

void
SDFSphere::setRadius(float radius)
{
    _radiusAndPadding.x = radius;
}
