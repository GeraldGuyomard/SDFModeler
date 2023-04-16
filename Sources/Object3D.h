//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include <memory>
#include "Serializer.h"
#include "SerializedWorld.h"
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
class World;
using WorldPtr = std::shared_ptr<World>;
using WorldWPtr = std::weak_ptr<World>;

class Object3DFactory
{
public:
    using Ptr = std::shared_ptr<Object3DFactory>;
    Object3DFactory(const std::string& name);
    
    virtual ~Object3DFactory() = default;
    
    const std::string& name() const { return _name; }
    virtual std::shared_ptr<Object3D> make(const WorldPtr&) const = 0;
    
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
    
    WorldPtr world() const { return _world.lock(); }
    
    void serialize(SerializedObjects&) const;
    virtual size_t selfSerialize(uint8_t* ptr) const = 0;
    
    ObjectID id() const { return _id; }
    void setId(ObjectID);
    
    Object3D::Ptr objectByID(ObjectID id) const;
    
    Material3D::Ptr material() const { return _material; }
    void setMaterial(const Material3D::Ptr&);
    
    MaterialID materialID() const;
    
    Ptr parent() const { return _parent.lock(); }
    
    float4x4 worldTransform() const;
    void setWorldTransform(const float4x4&);
    
    float4x4 localTransform() const { return _localTransform; }
    void setLocalTransform(const float4x4&);
    void setLocalTransform(const RSTTransformer&);
    
    bool selected() const { return _selected; }
    void setSelected(bool selected);
    
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
    Object3D(const WorldPtr& world);
    Object3D(const WorldPtr& world, ObjectID id);
    
private:
    
    virtual void serializeHierarchy(SerializedObjects& serializedObjects, uint8_t*& ptr) const;
    
    const WorldWPtr _world;
    
    ObjectID _id = 0;
    Material3D::Ptr _material;
    Operation _operation = Operation::addition;
    
    Object3D::WPtr _parent;
    std::vector<Ptr> _children;
    
    float4x4 _localTransform = float4x4_identity();
    bool _selected = false;
};

class Group3D : public Object3D
{
public:
    using _inherited = Object3D;
    Group3D(const WorldPtr&);
    
    size_t selfSerialize(uint8_t* ptr) const override;
    
protected:
    Group3D(const WorldPtr& world, ObjectID id);
};

template <typename TGeometry>
class TObject3D : public Object3D
{
public:
    using _inherited = Object3D;
    
    TObject3D(const WorldPtr& world, const TGeometry& geometry)
    : _inherited(world), _geometry(geometry)
    {}
    
    size_t selfSerialize(uint8_t* ptr) const final override
    {
        RSTTransformer transformer { worldTransform() };
        
        SDFObject<TGeometry, RSTTransformer> object { _geometry, transformer, materialID() };
        
        const bool selected = this->selected();
        if (selected)
        {
            object.setExtraCullingMargin(selected ? kOutlineThickness : 0.f);
        }
        
        return serializeObject<SDFObject<TGeometry, RSTTransformer>>(
                                            ptr,
                                           object,
                                           id(),
                                           object.objectType(),
                                           transformer.transformerType(),
                                           selected);
    }
    
private:
    TGeometry _geometry;
};

template <typename TGeometry>
class TObject3DFactory : public Object3DFactory
{
public:
    TObject3DFactory(const std::string& name, const TGeometry& geometry)
    : Object3DFactory(name), _geometry(geometry)
    {}
    
    std::shared_ptr<Object3D> make(const WorldPtr& world) const override
    {
        return std::make_shared<TObject3D<TGeometry>>(world, _geometry);
    }
    
private:
    const TGeometry _geometry;
};

template <typename TGeometry>
class TObject3DFactoryRegistration final
{
public:
    TObject3DFactoryRegistration(const std::string& name, const TGeometry& geometry)
    {
        auto factory = std::make_shared<TObject3DFactory<TGeometry>>(name, geometry);
        Object3DFactory::addFactory(factory);
    }
};

class World final : public std::enable_shared_from_this<World>
{
public:
    static WorldPtr make();
    
    void serialize(SerializedWorld&, Materials& materials) const;
    
    void addMaterial(const Material3D::Ptr&);
    Material3D::Ptr addMaterial(const float4& color);
    
    Object3D::Ptr rootObject() const { return _rootObject; }
    
    const Object3D::Ptr& selectedObject() const { return _selectedObject; }
    void setSelectedObject(const Object3D::Ptr&);
    
    CommandHistory& commandHistory() { return _commandHistory; }
    
    ObjectID generateNewObjectID();
    
private:
    World() = default;
    
    void init();
    
    class RootObject3D final : public Group3D
    {
    public:
        RootObject3D(const WorldPtr&, ObjectID id);
    };
    
    Object3D::Ptr _rootObject;
    
    ObjectID _nextAvailableObjectID = 1;
    std::vector<Material3D::Ptr> _materials;
    
    Object3D::Ptr _selectedObject;
    
    CommandHistory _commandHistory;
};
