//
//  SDFGeometry.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

enum class TransformerType : uint64_t
{
    translation = 0,
    translationRotation = 1,
    translationScaleRotation = 2
};

class Transformer final
{
public:
    
    template <typename TSDFGeometry>
    float computeSDF(TSDFGeometry primitive, float3 p) const;
    
    Ray localRay(Ray ray) const;
    
    TransformerType transformerType() const;
    
private:
    Transformer() = delete;
};

#if !defined(__METAL_VERSION__)

template <typename TSourceTransformer, typename TDestTransformer>
INLINE bool convert(const TSourceTransformer& src, TDestTransformer& dst)
{
    return false;
}

#endif
