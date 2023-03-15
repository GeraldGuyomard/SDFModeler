//
//  SDFObject.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Uniforms.h"
#include "SDFResult.h"

#include "Transformer/StandardTransformers.h"
#include "Material/ConstMaterial.h"
#include "Material/GridMaterial.h"

#include "SDFGeometry/SDFSphere.h"
#include "SDFGeometry/SDFPlane.h"
#include "SDFGeometry/SDFBox.h"
#include "SDFGeometry/SDFRoundedBox.h"
#include "SDFGeometry/SDFUnion.h"
#include "SDFGeometry/SDFSubstraction.h"

#include "SDFObject.h"

using Sphere = SDFObject<SDFSphere, RSTTransformer, ConstMaterial>;
using Plane = SDFObject<SDFPlane, RSTTransformer, ConstMaterial>;
using Grid = SDFObject<SDFPlane, RSTTransformer, GridMaterial>;
using Box = SDFObject<SDFBox, RSTTransformer, ConstMaterial>;
using RoundedBox = SDFObject<SDFRoundedBox, RSTTransformer, ConstMaterial>;

template <typename TShader>
class DynamicObject final
{
public:
    DynamicObject(CONSTANT DynamicScene& mutableState)
    : _dynamicScene(mutableState)
    {}
    
    bool culled() const
    {
        return false;
    }
    
    float computeDistance(float3 p) const
    {
        CONSTANT uint8_t* ptr = &_dynamicScene.buffer[0];
        CONSTANT uint8_t* end = ptr + _dynamicScene.size;
        
        while (ptr < end)
        {
            CONSTANT ObjectType* pType = (CONSTANT ObjectType*)ptr;
            
            const ObjectType type = *pType;
            ptr += sizeof(type);
            
            switch(type)
            {
                case ObjectType::sphere:
                {
                    CONSTANT Sphere* sphere = (CONSTANT Sphere*) ptr;
                    CONSTANT uint8_t* dataEnd = ptr + sizeof(Sphere);
                    
                    //return sphere->computeDistance(p);
                    
                    ptr = dataEnd;
                    break;
                }
                    
                case ObjectType::box:
                {
                    break;
                }
            }
        }
        
        return +1e5;
    }
    
    float4 computeAlbedo(float3 p) const
    {
        return { 1, 0, 0, 1};
    }
    
    SDFResult rayMarch(Ray ray, TShader shader) const
    {
        constexpr int kNbSteps = 100;
        
        float d = 0.f;
        
        for (int i=0; i < kNbSteps; ++i)
        {
            float3 p = ray.pt(d);
            auto result = computeSDF(p, ray, shader, *this);
            
            if (result.hit())
            {
                return result;
            }
            
            d += result.distance;
            
            if (d > ray.maxLength)
            {
                break;
            }
        }
        
        return {};
    }
    
private:
    CONSTANT DynamicScene& _dynamicScene;
};
