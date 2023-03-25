//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "Object3D.h"
#include "Composition.h"

template <typename TPrimitive1, typename TPrimitive2, typename TTransformer, typename TMaterial>
class TComposition3D : public Object3D
{
public:
    
    TComposition3D(CompositionOperation operation,
                   const TPrimitive1& p1,
                   const TPrimitive2& p2,
                   const TTransformer& transformer,
                   const TMaterial& material)
    : _operation(operation), _primitive1(p1), _primitive2(p2), _transformer(transformer), _material(material)
    {
        
    }
    
    void serialize(SerializedScene& serializedScene, uint8_t*& ptr) const override
    {
        ObjectHeader* const h = (ObjectHeader*) ptr;
        
        SDFSerializedComposition<TTransformer, TMaterial> serializedComp { _operation, _transformer, _material };
        copy(h, serializedComp);
        ptr += h->byteSize;
        
        // then copy the 2 primitives
        _copyPrimitive(ptr, _primitive1);
        _copyPrimitive(ptr, _primitive2);
        
        serializedScene.objectCount++;
    }
    
private:
    
    template <typename TPrimitive>
    static void _copyPrimitive(uint8_t*& ptr, const TPrimitive& prim)
    {
        auto h = (ObjectHeader*) ptr;
        copy(h, prim);
        ptr += h->byteSize;
    }
    
    CompositionOperation _operation;
    TPrimitive1 _primitive1;
    TPrimitive2 _primitive2;
    
    TTransformer _transformer;
    TMaterial _material;
};

template <typename TPrimitive1, typename TPrimitive2>
class Composition3D final : public TComposition3D<TPrimitive1, TPrimitive2, Composition::Transformer, Composition::Material>
{
public:
    using _inherited = TComposition3D<TPrimitive1, TPrimitive2, Composition::Transformer, Composition::Material>;
    
    using Transformer = Composition::Transformer;
    using Material = Composition::Material;
    
    Composition3D(CompositionOperation operation,
                   const TPrimitive1& p1,
                   const TPrimitive2& p2,
                   const Transformer& transformer,
                   const Material& material)
    : _inherited(operation, p1, p2, transformer, material)
    {}
};

