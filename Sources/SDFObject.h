//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Transformer/Transformer.h"

template <typename TGeometry, typename TTransformer>
class SDFObject final
{
    SDFObject(TGeometry geometry, TTransformer transformer)
    : _geometry(geometry), _transformer(transformer)
    {}
    
    float computeDistance(float3 p) const
    {
        return _geometry.computeDistance(p);
    }
    
    float3 transform(float3 p) const
    {
        return _transformer.transform(p);
    }
    
private:
    const TGeometry _geometry;
    const TTransformer _transformer;
};
