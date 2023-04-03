//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Object3D.h"
#include "Composition.h"

template <typename TTransformer, typename TMaterial>
class TComposition3D : public Object3D
{
public:
    
    TComposition3D(const TTransformer& transformer,
                   const TMaterial& material)
    : _transformer(transformer), _material(material)
    {}
    
    void addAdditiveObject(Object3D::Ptr object)
    {
        object->setId(id());
        _additiveObjects.push_back(std::move(object));
    }
    
    void addSubstractiveObject(Object3D::Ptr object)
    {
        _substractiveObjects.push_back(std::move(object));
    }
    
    ObjectType objectType() const override
    {
        return ObjectType::composition;
    }
    
    TransformerType transformerType() const override
    {
        return TTransformer::transformerType();
    }
    
    size_t serialize(uint8_t* ptr) const override
    {
        ObjectHeader* const header = (ObjectHeader*) ptr;
        
        SDFSerializedComposition<TTransformer, TMaterial> serializedComp
        {
            _additiveObjects.size(),
            _substractiveObjects.size(),
            _transformer,
            _material
        };
        
        copy(header, serializedComp, id(), ObjectType::composition, TTransformer::transformerType());
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
    
private:
    
    template <typename TPrimitive>
    static void _copyPrimitive(uint8_t*& ptr, const TPrimitive& prim)
    {
        auto h = (ObjectHeader*) ptr;
        copy(h, prim);
        ptr += h->byteSize;
    }
    
    TTransformer _transformer;
    TMaterial _material;
    
    std::vector<Object3D::Ptr> _additiveObjects;
    std::vector<Object3D::Ptr> _substractiveObjects;
};

class Composition3D final : public TComposition3D<Composition::Transformer, Composition::Material>
{
public:
    using _inherited = TComposition3D<Composition::Transformer, Composition::Material>;
    
    using Transformer = Composition::Transformer;
    using Material = Composition::Material;
    
    Composition3D(const Transformer& transformer, const Material& material)
    : _inherited(transformer, material)
    {}
};

