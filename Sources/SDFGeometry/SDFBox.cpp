//
//  SDFRoundedBox.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#include "SDFBox.h"
#include "Type.h"

template <>
void initializeType<SDFBox>(Type& type)
{
    type.addProperty<SDFBox, float, &SDFBox::width, &SDFBox::setWidth>("width");
    type.addProperty<SDFBox, float, &SDFBox::height, &SDFBox::setHeight>("height");
    type.addProperty<SDFBox, float, &SDFBox::depth, &SDFBox::setDepth>("depth");
}
