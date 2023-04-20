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
    
    BoundingBox(float3 minPoint, float3 maxPoint)
    : minPoint(minPoint), maxPoint(maxPoint)
    {}
    
    bool empty() const
    {
        return (minPoint.x > maxPoint.x) || (minPoint.y > maxPoint.y) || (minPoint.z > maxPoint.z);
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
