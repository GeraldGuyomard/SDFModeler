//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Headers.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"

#include "Transformer/StandardTransformers.h"

#include "TransformedGeometry.h"

#include "Material/SimpleMaterial.h"

// Concept of TransformedGeometryEvaluator
template <typename TTransformedGeometry, typename TReturnType>
class TransformedGeometryEvaluator
{
public:
    TransformedGeometryEvaluator() = delete;
    
    TReturnType evaluate(CONSTANT TransformedGeometryHeader*, TTransformedGeometry) const;
};

class DistanceEvaluator
{
public:
    
    DistanceEvaluator(float3 pt)
    : _pt(pt)
    {}
    
    template <typename TTransformedGeometry>
    float evaluate(CONSTANT TransformedGeometryHeader* header, TTransformedGeometry geometry) const
    {
        return geometry.computeDistance(_pt);
    }
    
private:
    const float3 _pt;
};

class CullEvaluator
{
public:
    CullEvaluator(Ray ray)
    : _ray(ray)
    {}
    
    template <typename TTransformedGeometry>
    bool evaluate(CONSTANT TransformedGeometryHeader* header, TTransformedGeometry geometry) const
    {
        return geometry.evaluateCulling(_ray);
    }
    
private:
    Ray _ray;
};

class CullEvaluatorWithExtraCullingMarginOverride
{
public:
    CullEvaluatorWithExtraCullingMarginOverride(Ray ray, float extraCullingMargin)
    : _ray(ray), _extraCullingMargin(extraCullingMargin)
    {}
    
    template <typename TTransformedGeometry>
    bool evaluate(CONSTANT TransformedGeometryHeader* header, TTransformedGeometry geometry) const
    {
        geometry.setExtraCullingMargin(_extraCullingMargin);
        return geometry.evaluateCulling(_ray);
    }
    
private:
    Ray _ray;
    float _extraCullingMargin;
};

template <typename TShader>
class ShadeEvaluator
{
public:
    
    ShadeEvaluator(Ray ray, float distance, float3 pt, TShader shader, MaterialID materialID)
    : _ray(ray), _distance(distance), _pt(pt), _shader(shader), _materialID(materialID)
    {}
    
    template <typename TTransformedGeometry>
    float4 evaluate(CONSTANT TransformedGeometryHeader* header, TTransformedGeometry geometry) const
    {
        return _shader.computeShade(geometry, _ray, _distance, _pt, _materialID);
    }
    
private:
    const Ray _ray;
    const float _distance;
    const float3 _pt;
    const TShader _shader;
    const MaterialID _materialID;
};

template <typename TEvaluator, typename TTransformedGeometry, typename TReturnValue>
INLINE TReturnValue evaluateTransformedGeometry(TEvaluator evaluator, CONSTANT TransformedGeometryHeader* header)
{
    CONSTANT TTransformedGeometry* geometry = transformedGeometry<TTransformedGeometry>(header);
    const TTransformedGeometry p = *geometry;
    return evaluator.evaluate(header, p);
}

template <typename TReturnValue>
struct EvaluationReturn
{
    const bool processed;
    TReturnValue returnValue;
    
    EvaluationReturn()
    : processed(false)
    {}
    
    EvaluationReturn(TReturnValue returnValue)
    : processed(true), returnValue(returnValue)
    {}
    
    operator bool() const
    {
        return processed;
    }
};

template <typename TEvaluator, typename TGeometry, typename TReturnValue>
INLINE EvaluationReturn<TReturnValue>
computeEvaluationReturn(TEvaluator evaluator, CONSTANT TransformedGeometryHeader* header)
{
    const auto code = header->transformedGeometryCode;
    
    if (code == computeTransformedGeometryCode<TGeometry, RSTTransformer>())
    {
        using TransformedGeom = TransformedGeometry<TGeometry, RSTTransformer>;
        return { evaluateTransformedGeometry<TEvaluator, TransformedGeom, TReturnValue>(evaluator, header) };
    }
    else if (code == computeTransformedGeometryCode<TGeometry, RTTransformer>())
    {
        using TransformedGeom = TransformedGeometry<TGeometry, RTTransformer>;
        return { evaluateTransformedGeometry<TEvaluator, TransformedGeom, TReturnValue>(evaluator, header) };
    }
    else if (code == computeTransformedGeometryCode<TGeometry, TranslationTransformer>())
    {
        using TransformedGeom = TransformedGeometry<TGeometry, TranslationTransformer>;
        return { evaluateTransformedGeometry<TEvaluator, TransformedGeom, TReturnValue>(evaluator, header) };
    }
    else
    {
        return {};
    }
}

template <typename TEvaluator, typename TReturnValue>
INLINE TReturnValue evaluateTransformedGeometry(TEvaluator evaluator, CONSTANT TransformedGeometryHeader* header)
{
    if (auto ret = computeEvaluationReturn<TEvaluator, SDFSphere, TReturnValue>(evaluator, header))
    {
        return ret.returnValue;
    }
    else if (auto ret = computeEvaluationReturn<TEvaluator, SDFBox, TReturnValue>(evaluator, header))
    {
        return ret.returnValue;
    }
    else if (auto ret = computeEvaluationReturn<TEvaluator, SDFRoundedBox, TReturnValue>(evaluator, header))
    {
        return ret.returnValue;
    }
    else if (auto ret = computeEvaluationReturn<TEvaluator, SDFPlane, TReturnValue>(evaluator, header))
    {
        return ret.returnValue;
    }
   
    return {};
}
