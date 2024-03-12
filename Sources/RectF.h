//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"

class RectF final
{
public:
    float2 topLeft = { 1e7f, 1e7f };
    float2 bottomRight = { -1e7f, -1e7f };
    
    RectF() = default;
    RectF(float2 topLeft, float2 bottomRight)
    : topLeft(topLeft), bottomRight(bottomRight)
    {}
    
    bool empty() const
    {
        return (topLeft.x > bottomRight.x) || (topLeft.y > bottomRight.y);
    }
    
    
    void add(float2 pt)
    {
        if (empty())
        {
            topLeft = bottomRight = pt;
        }
        else
        {
            topLeft = min(topLeft, pt);
            bottomRight = max(bottomRight, pt);
        }
    }
    
    bool contains(float2 pt) const
    {
        return (pt.x >= topLeft.x) && (pt.y >= topLeft.y)
        && (pt.x <= (bottomRight.x)) && (pt.y <= (bottomRight.y));
    }
    
    RectF makeIntersection(RectF other) const
    {
        const auto newTopLeft = max(topLeft, other.topLeft);
        const auto newBottomRight = min(bottomRight, other.bottomRight);
        
        return RectF { newTopLeft, newBottomRight };
    }
    
    bool intersects(RectF other) const
    {
        const auto inter = makeIntersection(other);
        return !inter.empty();
    }
};

