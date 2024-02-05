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
    float2 top = { 1e7f, 1e7f };
    float2 bottom = { -1e7f, -1e7f };
    
    RectF() = default;
    RectF(float2 top, float2 bottom)
    : top(top), bottom(bottom)
    {}
    
    bool empty() const
    {
        return (top.x > bottom.x) || (top.y > bottom.y);
    }
    
    
    void add(float2 pt)
    {
        if (empty())
        {
            top = bottom = pt;
        }
        else
        {
            top = min(top, pt);
            bottom = max(bottom, pt);
        }
    }
    
    bool contains(float2 pt) const
    {
        return (pt.x >= top.x) && (pt.y >= top.y)
        && (pt.x <= (bottom.x)) && (pt.y <= (bottom.y));
    }
    
    bool intersects(RectF other) const
    {
        const auto newTop = max(top, other.top);
        const auto newBottom = min(bottom, other.bottom);
        
        return (newTop.x <= newBottom.x) && (newTop.y <= newBottom.y);
    }
};

