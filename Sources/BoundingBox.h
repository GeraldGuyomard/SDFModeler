//
//  SDFModeler
//
//  Created by Gérald Guyomard on 3/5/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Ray.h"

class BoundingBox final
{
public:
    float3 minPoint;
    float3 maxPoint;
    
    BoundingBox()
    : minPoint { 1e7f, 1e7f, 1e7f }, maxPoint { -1e7f, -1e7f, -1e7f }
    {}
    
    BoundingBox(float3 minPoint, float3 maxPoint)
    : minPoint(minPoint), maxPoint(maxPoint)
    {}
    
    bool empty() const
    {
        return (minPoint.x > maxPoint.x) || (minPoint.y > maxPoint.y) || (minPoint.z > maxPoint.z);
    }
    
    void add(float3 pt)
    {
        minPoint = min(minPoint, pt);
        maxPoint = max(maxPoint, pt);
    }
    
    void add(BoundingBox other)
    {
        minPoint = min(minPoint, other.minPoint);
        maxPoint = max(maxPoint, other.maxPoint);
    }
    
    float3 center() const
    {
        return (minPoint + maxPoint) * 0.5f;
    }
    
    void points(float3 pts[8]) const
    {
        // Rear
        pts[0] = { -minPoint.x,  -minPoint.y, -minPoint.z };
        pts[1] = { -minPoint.x,  +minPoint.y, -minPoint.z };
        pts[2] = { +minPoint.x,  +minPoint.y, -minPoint.z };
        pts[3] = { +minPoint.x,  -minPoint.y, -minPoint.z };
        
        // Front
        pts[4] = { -minPoint.x,  -minPoint.y, +minPoint.z };
        pts[5] = { -minPoint.x,  +minPoint.y, +minPoint.z };
        pts[6] = { +minPoint.x,  +minPoint.y, +minPoint.z };
        pts[7] = { +minPoint.x,  -minPoint.y, +minPoint.z };
    }
    
    bool isCulled(float4x4 worldViewProjectionMatrix) const
    {
        float3 pts[8];
        points(pts);
        
        bool inFrustrum = false;
        
        inFrustrum |= isPointInFrustrum(pts[0], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[1], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[2], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[3], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[4], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[5], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[6], worldViewProjectionMatrix);
        inFrustrum |= isPointInFrustrum(pts[7], worldViewProjectionMatrix);
        
        return !inFrustrum;
    }
};

INLINE BoundingBox operator*(float4x4 m, BoundingBox box)
{
    float3 pts[8];
    box.points(pts);
    
    BoundingBox transformedBox;
    
    for (size_t i=0; i < 8; ++i)
    {
        float4 p = m * float4 { pts[i].x, pts[i].y, pts[i].z, 1.f };
        transformedBox.add(p.xyz);
    }
    
    return transformedBox;
}
