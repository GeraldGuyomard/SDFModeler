//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Transformer/Transformer.h"
#include "Material/Material.h"

template <typename TGeometry, typename TTransformer, typename TMaterial>
class SDFObject final
{
public:
    
    SDFObject(TGeometry geometry, TTransformer transformer = {}, TMaterial material = {})
    : _geometry(geometry), _transformer(transformer), _material(material)
    {}
    
    float computeDistance(float3 p) const
    {
        float3 transformedP = transform(p);
        return _geometry.computeDistance(transformedP);
    }
    
    float3 transform(float3 p) const
    {
        return _transformer.transform(p);
    }
    
    float4 computeAlbedo(float3 p) const
    {
        return _material.computeAlbedo(p);
    }
    
private:
    const TGeometry _geometry;
    const TTransformer _transformer;
    const TMaterial _material;
};
