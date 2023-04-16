//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Object3D.h"
#include "Composition.h"

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
    
    void serializeHierarchy(SerializedWorld& serializedWorld, uint8_t*& p) const override
    {
#if ENABLE_COMPOSITION
        const size_t size = selfSerialize(p);
        if (size != 0)
        {
            p += size;
            serializedWorld.objectCount++;
        }
#endif
    }
    
    size_t selfSerialize(uint8_t* ptr) const override
    {
#if ENABLE_COMPOSITION
        ObjectHeader* const header = (ObjectHeader*) ptr;
        
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
        SDFSerializedComposition<TTransformer> serializedComp
        {
            additiveObjects.size(),
            substractiveObjects.size(),
            transformer,
            materialID(),
            extraCullingMargin
        };
        
        copy(header, serializedComp, id(), ObjectType::composition, TTransformer::transformerType(), selected);
        ptr += header->byteSize;
        
        size_t subHeadersSize = 0;
        // then copy the primitives
        for (const auto& obj : additiveObjects)
        {
            const size_t s = obj->selfSerialize(ptr);
            subHeadersSize += s;
            ptr += s;
        }
        
        for (const auto& obj : substractiveObjects)
        {
            const size_t s = obj->selfSerialize(ptr);
            subHeadersSize += s;
            ptr += s;
        }
        
        // update the true header size that includes all the sub headers
        header->byteSize += subHeadersSize;
        
        return header->byteSize;
#else
        return 0;
#endif
    }
    
private:
    
#if ENABLE_COMPOSITION
    template <typename TPrimitive>
    static void _copyPrimitive(uint8_t*& ptr, const TPrimitive& prim)
    {
        auto h = (ObjectHeader*) ptr;
        copy(h, prim);
        ptr += h->byteSize;
    }
#endif
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

