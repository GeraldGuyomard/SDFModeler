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
#include "Culling.h"
#include "Ray.h"

template <typename TGeometry, typename TTransformer, typename TMaterial>
class SDFObject final
{
public:
    
    SDFObject(Ray ray, TGeometry geometry, TTransformer transformer = {}, TMaterial material = {})
    : _geometry(geometry),
    _transformer(transformer),
    _material(material)
    {
        const Ray localRay = _transformer.localRay(ray);
        _culled = evaluateCulling(_geometry, localRay);
    }
    
    bool culled() const
    {
        return _culled;
    }
    
    float computeDistance(float3 p) const
    {
        return _transformer.computeDistance(_geometry, p);
    }
    
    float4 computeAlbedo(float3 p) const
    {
        return _material.computeAlbedo(p);
    }
    
private:
    const TGeometry _geometry;
    const TTransformer _transformer;
    const TMaterial _material;
    
    bool _culled;
};

