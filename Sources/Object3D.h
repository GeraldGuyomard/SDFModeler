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
#include <set>

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
    using Ptr = std::shared_ptr<Object3D>;
    
    virtual ~Object3D() = default;
    
    virtual ObjectType objectType() const = 0;
    virtual TransformerType transformerType() const = 0;
    
    virtual size_t serialize(uint8_t* ptr) const = 0;
    
    ObjectID id() const { return _id; }
    void setId(ObjectID);
    
    Material3D::Ptr material() const { return _material; }
    void setMaterial(const Material3D::Ptr&);
    
    MaterialID materialID() const;
    
    virtual float4x4 transform() const = 0;
    virtual void setTransform(const float4x4&) = 0;
    
    virtual bool selected() const = 0;
    virtual void setSelected(bool selected) = 0;
    
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
    
    float4x4 transform() const override
    {
        return _primitive.transform();
    }
    
    void setTransform(const float4x4& transform) override
    {
        _primitive.setTransform(transform);
    }
    
    bool selected() const override
    {
        return _selected;
    }
    
    void setSelected(bool selected) override
    {
        _selected = selected;
        _primitive.setExtraCullingMargin(selected ? kOutlineThickness : 0.f);
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
            return serializeObject<TPrimitive>(ptr, _primitive, id(), objectType(), transformer.transformerType(), _selected);
        }
    }
    
private:
    
    void onMaterialChange() override
    {
        _primitive.setMaterialID(materialID());
    }
    
    TPrimitive _primitive;
    bool _selected = false;
};

class Object3DCollection final
{
public:
    
    Object3DCollection() = default;
    
    void addObject(Object3D::Ptr);
    
    void serialize(SerializedObjects&) const;
    
    bool empty() const { return _objects.empty(); }
    const std::vector<Object3D::Ptr>& objects() const { return _objects; }
    
    std::set<ObjectID> objectIDs() const;
    
    bool contains(const Object3D::Ptr&) const;
    
private:
    std::vector<Object3D::Ptr> _objects;
};

using Selection = Object3DCollection;

class World final
{
public:
    World();
    
    void serialize(SerializedWorld&, Materials& materials) const;
    
    void addMaterial(Material3D::Ptr);
    Material3D::Ptr addMaterial(const float4& color);
   
    void addObject(Object3D::Ptr);
    const std::vector<Object3D::Ptr>& objects() const;
    
    Object3D::Ptr objectByID(ObjectID id) const;
    
    const Selection& selection() const { return _selection; }
    void setSelection(const Selection&);
    
private:
    Object3DCollection _content;
    ObjectID _nextAvailableObjectID = 1;
    std::vector<Material3D::Ptr> _materials;
    
    Selection _selection;
};
