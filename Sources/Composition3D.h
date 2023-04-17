//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Object3D.h"

template <typename TTransformer>
class TComposition3D : public Object3D
{
public:
    using _inherited = Object3D;
    
    TComposition3D(const WorldPtr& world)
    : _inherited(world)
    {}
    
    void addChild(const Ptr& child) override
    {
        _inherited::addChild(child);
        child->setId(id());
    }
    
    void serializeHierarchy(SerializationContext& context) const override
    {
        selfSerialize(context);
    }
    
    void selfSerialize(SerializationContext& context) const override
    {
        auto header = context.currentCompoundObjectHeader;
        
        const TTransformer transformer { worldTransform() };
        
        std::vector<SimpleGeometryObject3D::Ptr> additiveObjects;
        std::vector<SimpleGeometryObject3D::Ptr> substractiveObjects;
        
        for (const auto& child : children())
        {
            auto object = std::dynamic_pointer_cast<SimpleGeometryObject3D>(child);
            if (object == nullptr)
            {
                continue;
            }
            
            switch (object->operation())
            {
                case SimpleGeometryObject3D::Operation::addition:
                {
                    additiveObjects.push_back(object);
                    break;
                }
                    
                case SimpleGeometryObject3D::Operation::substraction:
                {
                    substractiveObjects.push_back(object);
                    break;
                }
                    
                default: break;
            }
        }
        
        const bool selected = this->selected();
        
        const ObjectHeader objectHeader { id(), uint32_t(materialID()), selected };
        *header = { objectHeader, additiveObjects.size(), substractiveObjects.size() };
        
        // now add the geometries
        const float extraCullingMargin = selected ? kOutlineThickness : 0;
        
        uint32_t* indices = &header->firstPositiveIndex;
        
        for (const auto& additiveObject : additiveObjects)
        {
            const auto geomIndex = additiveObject->serializeGeometry(context);
            *(indices++) = geomIndex;
        }
        
        for (const auto& subtractiveObject : substractiveObjects)
        {
            const auto geomIndex = subtractiveObject->serializeGeometry(context);
            *(indices++) = geomIndex;
        }
        
        const uint8_t* start = reinterpret_cast<const uint8_t*>(header);
        const uint8_t* end = reinterpret_cast<const uint8_t*>(indices);
        const size_t size = end - start;
        
        header->byteSize = alignedSize(size);
        
        context.serializedWorld.compoundObjectsCount++;
    }
};

class Composition3D final : public TComposition3D<RSTTransformer>
{
public:
    using _inherited = TComposition3D<RSTTransformer>;
    
    using Transformer = RSTTransformer;
    
    Composition3D(const WorldPtr& world)
    : _inherited(world)
    {}
};

