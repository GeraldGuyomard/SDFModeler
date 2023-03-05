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

template <typename TGeometry, typename TTransformer, typename TMaterial>
class SDFObject final
{
public:
    
    SDFObject(TGeometry geometry, TTransformer transformer = {}, TMaterial material = {})
    : _geometry(geometry), _transformer(transformer), _material(material)
    {}
    
    void setCulling(Ray ray)
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
    
    bool _culled = false;
};

template <typename TFirstSDFObject>
void setCulling(Ray ray, TFirstSDFObject first)
{
    first.setCulling(ray);
}

template <typename TFirstSDFObject, typename... TSDFObjects>
void setCulling(Ray ray, TFirstSDFObject first, TSDFObjects... others)
{
    setCulling(ray, first);
    setCulling(ray, others...);
}
