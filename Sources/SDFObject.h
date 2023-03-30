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
    
    using Geometry = TGeometry;
    using Transformer = TTransformer;
    
    SDFObject(TGeometry geometry, TTransformer transformer = {}, TMaterial material = {})
    : _geometry(geometry),
    _transformer(transformer),
    _material(material)
    {}
    
    static ObjectType objectType()
    {
        return TGeometry::objectType();
    }
    
    static TransformerType transformerType()
    {
        return TTransformer::transformerType();
    }
    
    bool evaluateCulling(Ray ray) const
    {
        const Ray localRay = _transformer.localRay(ray);
        return _geometry.evaluateCulling(localRay);
    }
    
    float computeDistance(float3 p) const
    {
        return _transformer.computeDistance(_geometry, p);
    }
    
    float4 computeAlbedo(float3 p) const
    {
        return _material.computeAlbedo(p);
    }
    
    TGeometry geometry() const { return _geometry; }
    TTransformer transformer() const { return _transformer; }
    TMaterial material() const { return _material; }
    
private:
    TGeometry _geometry;
    TTransformer _transformer;
    TMaterial _material;
};

