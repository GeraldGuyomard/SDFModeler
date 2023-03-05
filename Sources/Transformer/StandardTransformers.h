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
    
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        return primitive.computeDistance(p - _translation);
    }
    
private:
    const float3 _translation = { 0.f };
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
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        p = _invRotTransform * (p - _translation);
        return primitive.computeDistance(p);
    }
    
private:
    const float3x3 _invRotTransform = float3x3_identity();
    const float3 _translation = { 0.f };
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
    
    template <typename TSDFGeometry>
    float computeDistance(TSDFGeometry primitive, float3 p) const
    {
        const auto localPBeforeScale = _invRotTransform * (p - _translation);
        return primitive.computeDistance(localPBeforeScale / _scale) * _scale;
    }
    
private:
    const float3x3 _invRotTransform = float3x3_identity();
    const float3 _translation = { 0.f };
    const float _scale = 1.f;
};
