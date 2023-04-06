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

static constexpr CONSTANT float kOutlineThickness = 2.5e-2f;

template <typename TGeometry, typename TTransformer>
class SDFObject final
{
public:
    
    using Geometry = TGeometry;
    using Transformer = TTransformer;
    
    SDFObject(TGeometry geometry,
              TTransformer transformer = {},
              MaterialID materialID = 0)
    : _geometry(geometry),
    _transformer(transformer),
    _materialID(materialID)
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
        return _geometry.evaluateCulling(localRay, _extraCullingMargin);
    }
    
    float computeDistance(float3 p) const
    {
        return _transformer.computeDistance(_geometry, p);
    }
    
    float raycast(Ray ray) const
    {
        const Ray localRay = _transformer.localRay(ray);
        return _geometry.raycast(localRay);
    }
    
    TGeometry geometry() const { return _geometry; }
    TTransformer transformer() const { return _transformer; }
    
    MaterialID materialID() const { return _materialID; }
    void setMaterialID(MaterialID id) { _materialID = id; }
    
    float extraCullingMargin() const { return _extraCullingMargin; }
    void setExtraCullingMargin(float m) { _extraCullingMargin = m; }
    
    float4x4 transform() const
    {
        return _transformer.transform();
    }
    
    void setTransform(float4x4 transform)
    {
        return _transformer.setTransform(transform);
    }
    
private:
    TGeometry _geometry;
    TTransformer _transformer;
    MaterialID _materialID;
    float _extraCullingMargin = 0;
};

