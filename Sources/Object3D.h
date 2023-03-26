//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include <memory>
#include "Serializer.h"
#include "Scene.h"
#include <vector>

class Object3D
{
public:
    using Ptr = std::unique_ptr<Object3D>;
    
    virtual ~Object3D() = default;
    
    virtual void serialize(uint8_t*& ptr) const = 0;
};

template <typename TPrimitive>
class TObject3D : public Object3D
{
public:
    TObject3D(const TPrimitive& prim)
    : _primitive(prim)
    {}
    
    void serialize(uint8_t*& ptr) const override
    {
        serializeObject<TPrimitive>(ptr, _primitive);
    }
    
private:
    TPrimitive _primitive;
};

class World final
{
public:
    World() = default;
    
    void addObject(Object3D::Ptr);
    
    void serialize(SerializedScene&) const;
    
private:
    std::vector<Object3D::Ptr> _objects;
};
