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
#include <string>

#include "Command.h"

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

class Object3D;

class Object3DFactory
{
public:
    using Ptr = std::shared_ptr<Object3DFactory>;
    Object3DFactory(const std::string& name);
    
    virtual ~Object3DFactory() = default;
    
    const std::string& name() const { return _name; }
    virtual std::shared_ptr<Object3D> make() const = 0;
    
    // Factory
    static void addFactory(const Ptr&);
    static const std::vector<Ptr>& factories();
    
private:
    const std::string _name;
};

class Object3D : public std::enable_shared_from_this<Object3D>
{
public:
    using Ptr = std::shared_ptr<Object3D>;
    using WPtr = std::weak_ptr<Object3D>;
    
    virtual ~Object3D() = default;
    
    virtual ObjectType objectType() const = 0;
    virtual TransformerType transformerType() const = 0;
    
    virtual size_t serialize(uint8_t* ptr) const = 0;
    
    ObjectID id() const { return _id; }
    void setId(ObjectID);
    
    Material3D::Ptr material() const { return _material; }
    void setMaterial(const Material3D::Ptr&);
    
    MaterialID materialID() const;
    
    Ptr parent() const { return _parent.lock(); }
    
    float4x4 worldTransform() const;
    void setWorldTransform(const float4x4&);
    
    virtual float4x4 localTransform() const = 0;
    virtual void setLocalTransform(const float4x4&) = 0;
    
    virtual bool selected() const = 0;
    virtual void setSelected(bool selected) = 0;
    
    const std::vector<Ptr>& children() const { return _children; }
    virtual void addChild(const Ptr& child);
    void removeFromParent();
    
    enum class Operation
    {
        addition,
        substraction
    };
    
    Operation operation() const { return _operation; }
    void setOperation(Operation);
    
protected:
    virtual void onMaterialChange() {}
    
private:
    ObjectID _id = 0;
    Material3D::Ptr _material;
    Operation _operation = Operation::addition;
    
    Object3D::WPtr _parent;
    std::vector<Ptr> _children;
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
    
    float4x4 localTransform() const override
    {
        return _primitive.transform();
    }
    
    void setLocalTransform(const float4x4& transform) override
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
        typename TPrimitive::Transformer transformer { worldTransform() };
        
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
            auto primitive = _primitive;
            primitive.setTransformer(transformer);
            
            return serializeObject<TPrimitive>(ptr, primitive, id(), objectType(), transformer.transformerType(), _selected);
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

template <typename TPrimitive>
class TObject3DFactory : public Object3DFactory
{
public:
    TObject3DFactory(const std::string& name, const TPrimitive& primitive)
    : Object3DFactory(name), _object(primitive)
    {}
    
    using Object = SDFObject<TPrimitive, RSTTransformer>;
    
    std::shared_ptr<Object3D> make() const override
    {
        return std::make_shared<TObject3D<Object>>(_object);
    }
    
private:
    const SDFObject<TPrimitive, RSTTransformer> _object;
};

template <typename TPrimitive>
class TObject3DFactoryRegistration final
{
public:
    TObject3DFactoryRegistration(const std::string& name, const TPrimitive& primitive)
    {
        auto factory = std::make_shared<TObject3DFactory<TPrimitive>>(name, primitive);
        Object3DFactory::addFactory(factory);
    }
};

class Object3DCollection final
{
public:
    
    Object3DCollection() = default;
    
    void addObject(const Object3D::Ptr&);
    void removeObject(const Object3D::Ptr&);
    
    void serialize(SerializedObjects&) const;
    
    bool empty() const { return _objects.empty(); }
    const std::vector<Object3D::Ptr>& objects() const { return _objects; }
    Object3D::Ptr anyObject() const;
    
    std::set<ObjectID> objectIDs() const;
    
    bool contains(const Object3D::Ptr&) const;
    
private:
    std::vector<Object3D::Ptr> _objects;
};

class World final
{
public:
    World();
    
    void serialize(SerializedWorld&, Materials& materials) const;
    
    void addMaterial(const Material3D::Ptr&);
    Material3D::Ptr addMaterial(const float4& color);
   
    void addObject(const Object3D::Ptr&);
    void removeObject(const Object3D::Ptr&);
    const std::vector<Object3D::Ptr>& objects() const;
    
    Object3D::Ptr objectByID(ObjectID id) const;
    
    const Object3D::Ptr& selectedObject() const { return _selectedObject; }
    void setSelectedObject(const Object3D::Ptr&);
    
    CommandHistory& commandHistory() { return _commandHistory; }
    
private:
    
    Object3DCollection _content;
    ObjectID _nextAvailableObjectID = 1;
    std::vector<Material3D::Ptr> _materials;
    
    Object3D::Ptr _selectedObject;
    
    CommandHistory _commandHistory;
};
