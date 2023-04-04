//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "ObjectHeader.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"
#include "SDFGeometry/SDFSubstraction.h"

#include "SDFObject.h"

#include "Transformer/StandardTransformers.h"
#include "Material/ConstMaterial.h"
#include "Material/GridMaterial.h"

// Concept of PrimitiveEvaluator
template <typename TPrimitive, typename TReturnType>
class PrimitiveEvaluator
{
public:
    PrimitiveEvaluator() = delete;
    
    TReturnType evaluate(CONSTANT ObjectHeader*, TPrimitive) const;
};

class DistanceEvaluator
{
public:
    
    DistanceEvaluator(float3 pt)
    : _pt(pt)
    {}
    
    template <typename TPrimitive>
    float evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive) const
    {
        return primitive.computeDistance(_pt);
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
    
    template <typename TPrimitive>
    bool evaluate(CONSTANT ObjectHeader* header, TPrimitive prim) const
    {
        return prim.evaluateCulling(_ray);
    }
    
private:
    Ray _ray;
};

template <typename TShader>
class ShadeEvaluator
{
public:
    
    ShadeEvaluator(Ray ray, float distance, float3 pt, TShader shader)
    : _ray(ray), _distance(distance), _pt(pt), _shader(shader)
    {}
    
    template <typename TPrimitive>
    float4 evaluate(CONSTANT ObjectHeader* header, TPrimitive primitive) const
    {
        return _shader.computeShade(primitive, _ray, _distance, _pt);
    }
    
private:
    const Ray _ray;
    const float _distance;
    const float3 _pt;
    const TShader _shader;
};

using Sphere = SDFObject<SDFSphere, RSTTransformer>;
using Plane = SDFObject<SDFPlane, RSTTransformer>;
using Grid = SDFObject<SDFPlane, RSTTransformer>;
using Box = SDFObject<SDFBox, RSTTransformer>;
using RoundedBox = SDFObject<SDFRoundedBox, RSTTransformer>;

template <typename TEvaluator, typename TPrimitive, typename TReturnValue>
INLINE TReturnValue evaluateTypedPrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    CONSTANT TPrimitive* prim = typedPrimitive<TPrimitive>(header);
    const TPrimitive p = *prim;
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

template <typename TEvaluator, typename TPrimitive, typename TReturnValue>
INLINE EvaluationReturn<TReturnValue>
computeEvaluationReturn(TEvaluator evaluator, CONSTANT ObjectHeader* header)
{
    const auto objectCode = header->objectCode;
    
    if (objectCode == computeObjectCode<TPrimitive, RSTTransformer>())
    {
        using Object = SDFObject<TPrimitive, RSTTransformer>;
        return { evaluateTypedPrimitive<TEvaluator, Object, TReturnValue>(evaluator, header) };
    }
    else if (objectCode == computeObjectCode<TPrimitive, RTTransformer>())
    {
        using Object = SDFObject<TPrimitive, RTTransformer>;
        return { evaluateTypedPrimitive<TEvaluator, Object, TReturnValue>(evaluator, header) };
    }
    else if (objectCode == computeObjectCode<TPrimitive, TranslationTransformer>())
    {
        using Object = SDFObject<TPrimitive, TranslationTransformer>;
        return { evaluateTypedPrimitive<TEvaluator, Object, TReturnValue>(evaluator, header) };
    }
    else
    {
        return {};
    }
}

template <typename TEvaluator, typename TReturnValue>
INLINE TReturnValue evaluateAtomicPrimitive(TEvaluator evaluator, CONSTANT ObjectHeader* header)
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
    /*else if (auto ret = computeEvaluationReturn<TEvaluator, SDFGrid, TReturnValue>(evaluator, header))
    {
        return ret.returnValue;
    }*/
   
    return {};
}
