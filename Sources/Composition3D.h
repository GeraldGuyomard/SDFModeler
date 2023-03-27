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
    
    void serialize(uint8_t*& ptr) const override
    {
        ObjectHeader* const h = (ObjectHeader*) ptr;
        
        SDFSerializedComposition<TTransformer, TMaterial> serializedComp
        {
            _additiveObjects.size(),
            _substractiveObjects.size(),
            _transformer,
            _material
        };
        
        copy(h, serializedComp, ObjectType::composition);
        ptr += h->byteSize;
        
        // then copy the primitives
        for (const auto& obj : _additiveObjects)
        {
            obj->serialize(ptr);
        }
        
        for (const auto& obj : _substractiveObjects)
        {
            obj->serialize(ptr);
        }
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

