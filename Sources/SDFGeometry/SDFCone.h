//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//


#pragma once

#include "SDFGeometry/SDFGeometry.h"
#include "Culling.h"
#include "ObjectType.h"

class SDFCone final
{
public:

    static ObjectType objectType() { return ObjectType::cone; }

    SDFCone(float radius, float height)
    : _params { radius, height, 0.f, 0.f }
    {}

    float radius() const { return _params.x; }
    float height() const { return _params.y; }

    float computeDistance(float3 p) const
    {
        // Cone: tip at y=+height/2, base at y=-height/2 with given radius.
        // Remap so tip is at origin and base is at y=-height.
        float h = _params.y;
        float r = _params.x;

        // IQ's exact capped-cone SDF.
        // q = base-edge point in (radial, axial) space: (r, -h)
        // Shift p.y so tip is at origin, base at y=-h
        float2 q = { r, -h };
        float2 w = { length(p.xz), p.y - h * 0.5f };

        float2 a = w - q * clamp(dot(w, q) / dot(q, q), 0.f, 1.f);
        float2 b = w - q * float2 { clamp(w.x / q.x, 0.f, 1.f), 1.f };
        float k = sign(q.y); // -1
        float d = min(dot(a, a), dot(b, b));
        float s = max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));

        return sqrt(d) * sign(s);
    }

    bool evaluateCulling(Ray ray, float outlineThickness) const
    {
        return evaluateBoxCulling(halfSize(), ray, outlineThickness);
    }

    BoundingBox boundingBox() const
    {
        const float3 hs = halfSize();
        return { -hs, hs };
    }

    float3 halfSize() const
    {
        return { radius(), height() * 0.5f, radius() };
    }

    void setRadius(float);
    void setHeight(float);

private:
    float4 _params;
};
