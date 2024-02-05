//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"

class Rect final
{
public:
    float2 origin = { 0.f, 0.f };
    float2 size = { 0.f, 0.f };
    
    Rect() = default;
    Rect(float2 origin, float2 size)
    : origin(origin), size(size)
    {}
    
    bool empty() const
    {
        return (size.x <= 0.f) || (size.y <= 0.f);
    }
    
    float2 bottom() const
    {
        return origin + size;
    }
    
    void add(float2 pt)
    {
        const auto b = max(bottom(), pt);
        origin = min(origin, pt);
        size = b - pt;
    }
    
    bool contains(float2 pt) const
    {
        return (pt.x >= origin.x) && (pt.y >= origin.y)
        && (pt.x <= (origin.x + size.x)) && (pt.y <= (origin.y + size.y));
    }
};
