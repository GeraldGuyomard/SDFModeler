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
        return;
        
        auto header = context.currentCompoundObjectHeader;
        
        const TTransformer transformer { worldTransform() };
        
        std::vector<Object3D::Ptr> additiveObjects;
        std::vector<Object3D::Ptr> substractiveObjects;
        
        for (const auto& child : children())
        {
            switch (child->operation())
            {
                case Operation::addition:
                {
                    additiveObjects.push_back(child);
                    break;
                }
                    
                case Operation::substraction:
                {
                    substractiveObjects.push_back(child);
                    break;
                }
                    
                default: break;
            }
        }
        
        const bool selected = this->selected();
        
        float extraCullingMargin = selected ? kOutlineThickness : 0;
        
        const ObjectHeader objectHeader { id(), uint32_t(materialID()), selected };
        *header = { objectHeader, additiveObjects.size(), substractiveObjects.size() };
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

