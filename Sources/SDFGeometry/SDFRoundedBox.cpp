//
//  SDFRoundedBox.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#include "SDFRoundedBox.h"
#include "Type.h"

template <>
void initializeType<SDFRoundedBox>(Type& type)
{
    type.addProperty<SDFRoundedBox, float, &SDFRoundedBox::width, &SDFRoundedBox::setWidth>("width");
    type.addProperty<SDFRoundedBox, float, &SDFRoundedBox::height, &SDFRoundedBox::setHeight>("height");
    type.addProperty<SDFRoundedBox, float, &SDFRoundedBox::depth, &SDFRoundedBox::setDepth>("depth");
    type.addProperty<SDFRoundedBox, float, &SDFRoundedBox::radius, &SDFRoundedBox::setRadius>("radius");
}
