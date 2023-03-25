//
//  SDFUnion.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/27/23.
//

#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "ObjectType.h"

template<typename P1, typename P2>
class SDFUnion final
{
public:
    SDFUnion(P1 p1, P2 p2)
    : _p1(p1), _p2(p2)
    {}
    
    float computeDistance(float3 p) const
    {
        if (_p1.culled())
        {
            return _p2.computeDistance(p);
        }
        else if (_p2.culled())
        {
            return _p1.computeDistance(p);
        }
        else
        {
            const float d1 = _p1.computeDistance(p);
            const float d2 = _p2.computeDistance(p);
            return min(d1, d2);
        }
    }
    
    bool evaluateCulling(Ray ray) const
    {
        return _p1.culled() && _p2.culled();
    }
    
private:
    const P1 _p1;
    const P2 _p2;
};

