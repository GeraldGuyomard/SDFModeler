//
//  SDFUnion.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "ObjectType.h"

template<typename P, typename PToRemove>
class SDFSubstraction final
{
public:
    SDFSubstraction(P p, PToRemove toRemove)
    : _p(p), _toRemove(toRemove)
    {}
    
    float computeDistance(float3 p) const
    {
        return max(-_toRemove.computeDistance(p), _p.computeDistance(p));
    }
    
    bool evaluateCulling(Ray ray) const
    {
        return _p.evaluateCulling(ray);
    }
    
private:
    const P _p;
    const PToRemove _toRemove;
};

