//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "Transformer/Transformer.h"

class TranslationTransformer final
{
public:
    
    TranslationTransformer() = default;
    
    TranslationTransformer(float3 translation)
    : _translation(translation)
    {}
    
    static TransformerType transformerType()
    {
        return TransformerType::translation;
    }
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        return primitive.computeDistance(p - _translation);
    }
    
    Ray localRay(Ray ray) const
    {
        Ray r = ray;
        r.origin -= _translation;
        return r;
    }
    
private:
    float3 _translation = { 0.f };
    float _padding;
};

class RTTransformer final
{
public:
    
    RTTransformer() = default;
    
    RTTransformer(float3 translation)
    : _translation(translation)
    {}
    
    RTTransformer(float3 translation, float3 rotationAxis, float angle)
    : _translation(translation), _invRotTransform(matrix3x3_rotation(-angle, rotationAxis))
    {}
    
    RTTransformer(float3 translation, float3x3 invRotTransform)
    : _translation(translation), _invRotTransform(invRotTransform)
    {}
    
    static TransformerType transformerType()
    {
        return TransformerType::translationRotation;
    }
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        p = _invRotTransform * (p - _translation);
        return primitive.computeDistance(p);
    }
    
    Ray localRay(Ray ray) const
    {
        Ray r = ray;
        r.origin = _invRotTransform * (ray.origin - _translation);
        r.direction = _invRotTransform * ray.direction;
        r.direction = normalize(r.direction);
        
        return r;
    }
    
private:
    float3x3 _invRotTransform = float3x3_identity();
    float3 _translation = { 0.f };
    float _padding;
};


class RSTTransformer final
{
public:
    
    RSTTransformer() = default;
    
    RSTTransformer(float3 translation)
    : _translation(translation)
    {}
    
    RSTTransformer(float3 translation, float3 rotationAxis, float angle, float scale = 1.f)
    : _translation(translation), _invRotTransform(matrix3x3_rotation(-angle, rotationAxis)), _scale(scale)
    {}
    
    RSTTransformer(float4x4 transform);
    
    static TransformerType transformerType()
    {
        return TransformerType::translationScaleRotation;
    }
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        const auto localPBeforeScale = _invRotTransform * (p - _translation);
        return primitive.computeDistance(localPBeforeScale / _scale) * _scale;
    }
    
    Ray localRay(Ray ray) const
    {
        Ray r = ray;
        r.origin = _invRotTransform * ((ray.origin - _translation) / _scale);
        r.direction = _invRotTransform * ray.direction;
        r.direction = normalize(r.direction);
        
        return r;
    }
    
    float3x3 invRotTransform() const { return _invRotTransform; }
    float3 translation() const { return _translation; }
    float scale() const { return _scale; }
    void setScale(float);
    
    float4x4 transform() const;
    void setTransform(float4x4);
    
    // For editing
    void computeEulers(DEVICE float& xAngle, DEVICE float& yAngle, DEVICE float& zAngle) const;
    
private:
    float3x3 _invRotTransform = float3x3_identity();
    float3 _translation = { 0.f };
    float _scale = 1.f;
};

#if !defined(__METAL_VERSION__)

template <>
INLINE bool convert<RSTTransformer, RTTransformer>(CONSTANT RSTTransformer& src, DEVICE RTTransformer& dst)
{
    if (src.scale() == 1.f)
    {
        dst = { src.translation(), src.invRotTransform() };
        return true;
    }
    
    return false;
}

template <>
INLINE bool convert<RSTTransformer, TranslationTransformer>(CONSTANT RSTTransformer& src, DEVICE TranslationTransformer& dst)
{
    if ((src.scale() == 1.f) && (src.invRotTransform() == float3x3_identity()))
    {
        dst = { src.translation() };
        return true;
    }
    
    return false;
}

#endif
