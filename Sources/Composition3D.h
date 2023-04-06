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
    
    TComposition3D(const TTransformer& transformer)
    : _transformer(transformer)
    {}
    
    void addAdditiveObject(Object3D::Ptr object)
    {
        object->setId(id());
        _additiveObjects.push_back(object);
        
        object->setParent(shared_from_this());
    }
    
    void addSubstractiveObject(Object3D::Ptr object)
    {
        _substractiveObjects.push_back(object);
        object->setParent(shared_from_this());
    }
    
    ObjectType objectType() const override
    {
        return ObjectType::composition;
    }
    
    TransformerType transformerType() const override
    {
        return TTransformer::transformerType();
    }
    
    float4x4 localTransform() const override
    {
        return _transformer.transform();
    }
    
    void setLocalTransform(const float4x4& transform) override
    {
        _transformer.setTransform(transform);
    }
    
    size_t serialize(uint8_t* ptr) const override
    {
        ObjectHeader* const header = (ObjectHeader*) ptr;
        
        const TTransformer transformer { worldTransform() };
        
        float extraCullingMargin = _selected ? kOutlineThickness : 0;
        SDFSerializedComposition<TTransformer> serializedComp
        {
            _additiveObjects.size(),
            _substractiveObjects.size(),
            transformer,
            materialID(),
            extraCullingMargin
        };
        
        copy(header, serializedComp, id(), ObjectType::composition, TTransformer::transformerType(), _selected);
        ptr += header->byteSize;
        
        size_t subHeadersSize = 0;
        // then copy the primitives
        for (const auto& obj : _additiveObjects)
        {
            const size_t s = obj->serialize(ptr);
            subHeadersSize += s;
            ptr += s;
        }
        
        for (const auto& obj : _substractiveObjects)
        {
            const size_t s = obj->serialize(ptr);
            subHeadersSize += s;
            ptr += s;
        }
        
        // update the true header size that includes all the sub headers
        header->byteSize += subHeadersSize;
        
        return header->byteSize;
    }
    
    bool selected() const override { return _selected; }
    void setSelected(bool selected) override { _selected = selected; }
    
private:
    
    template <typename TPrimitive>
    static void _copyPrimitive(uint8_t*& ptr, const TPrimitive& prim)
    {
        auto h = (ObjectHeader*) ptr;
        copy(h, prim);
        ptr += h->byteSize;
    }
    
    TTransformer _transformer;
    
    std::vector<Object3D::Ptr> _additiveObjects;
    std::vector<Object3D::Ptr> _substractiveObjects;
    bool _selected = false;
};

class Composition3D final : public TComposition3D<Composition::Transformer>
{
public:
    using _inherited = TComposition3D<Composition::Transformer>;
    
    using Transformer = Composition::Transformer;
    
    Composition3D(const Transformer& transformer = {})
    : _inherited(transformer)
    {}
};

