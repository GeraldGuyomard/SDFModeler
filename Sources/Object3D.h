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

class Material3D final
{
public:
    using Ptr = std::shared_ptr<Material3D>;
    
    Material3D(const SimpleMaterial& m);
    
    MaterialID id() const { return _id; }
    void setId(MaterialID);
    
    SimpleMaterial& material() { return _material; }
    
private:
    MaterialID _id = kNoMaterialID;
    SimpleMaterial _material;
};

class Object3D
{
public:
    using Ptr = std::unique_ptr<Object3D>;
    
    virtual ~Object3D() = default;
    
    virtual ObjectType objectType() const = 0;
    virtual TransformerType transformerType() const = 0;
    
    virtual size_t serialize(uint8_t* ptr) const = 0;
    
    ObjectID id() const { return _id; }
    void setId(ObjectID);
    
    Material3D::Ptr material() const { return _material; }
    void setMaterial(const Material3D::Ptr&);
    
    MaterialID materialID() const;
    
protected:
    virtual void onMaterialChange() {}
    
private:
    ObjectID _id = 0;
    Material3D::Ptr _material;
};

template <typename TPrimitive>
class TObject3D : public Object3D
{
public:
    TObject3D(const TPrimitive& prim)
    : _primitive(prim)
    {}
    
    ObjectType objectType() const override
    {
        return TPrimitive::objectType();
    }
    
    TransformerType transformerType() const override
    {
        return TPrimitive::Transformer::transformerType();
    }
    
    size_t serialize(uint8_t* ptr) const final override
    {
        const auto transformer = _primitive.transformer();
        
        /*const auto geometry = _primitive.geometry();
        const auto material = _primitive.material();
        
        RTTransformer rtTransformer;
        TranslationTransformer translationTransformer;
        
        if (convert<typename TPrimitive::Transformer, TranslationTransformer>(transformer, translationTransformer))
        {
            SDFObject object { geometry, translationTransformer, material };
            return serializeObject(ptr, object, object.objectType(), object.transformerType());
        }
        else if (convert<typename TPrimitive::Transformer, RTTransformer>(transformer, rtTransformer))
        {
            SDFObject object { geometry, rtTransformer, material };
            return serializeObject(ptr, object, object.objectType(), object.transformerType());
        }
        else*/
        {
            return serializeObject<TPrimitive>(ptr, _primitive, id(), objectType(), transformer.transformerType());
        }
    }
    
private:
    
    void onMaterialChange() override
    {
        _primitive.setMaterialID(materialID());
    }
    
    TPrimitive _primitive;
};

class Object3DCollection final
{
public:
    
    Object3DCollection() = default;
    
    void addObject(Object3D::Ptr);
    
    void serialize(SerializedObjects&) const;
    
private:
    std::vector<Object3D::Ptr> _objects;
};


class World final
{
public:
    World();
    
    void serialize(SerializedWorld&, Materials& materials) const;
    
    void addMaterial(Material3D::Ptr);
    Material3D::Ptr addMaterial(const float4& color);
   
    void addObject(Object3D::Ptr);
    
private:
    Object3DCollection _content;
    ObjectID _nextAvailableObjectID = 1;
    
    std::vector<Material3D::Ptr> _materials;
};
