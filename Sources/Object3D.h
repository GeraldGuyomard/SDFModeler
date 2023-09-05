//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include <memory>
#include "Serializer.h"
#include "SerializedWorldObject.h"
#include <vector>
#include <set>
#include <string>

#include "Command.h"
#include "BoundingBox.h"

#include "SerializationContext.h"
#include "Type.h"

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

class Type;

class Object3D : public std::enable_shared_from_this<Object3D>
{
public:
    using Ptr = std::shared_ptr<Object3D>;
    using WPtr = std::weak_ptr<Object3D>;
    
    Object3D(const WorldPtr& world);
    virtual ~Object3D() = default;
    
    WorldPtr world() const { return _world.lock(); }
    
    virtual const Type* type() const;
    
    virtual const Type* geometryType() const { return nullptr; }
    virtual void* geometry() { return nullptr; }
    
    // return true if self has been serialized
    bool serializeHierarchy(SerializationContext&) const;
    
    virtual void selfSerialize(SerializationContext&) const;
    
    virtual bool isCulled(const float4x4& viewProjectionMatrix) const { return true; }
    
    ObjectID id() const;
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
    
    virtual BoundingBox localBoundingBox() const { return {}; }
    
    BoundingBox boundingBoxInCoordinateFrame(const float4x4& coordinateFrame) const;
    BoundingBox boundingBoxOfHierarchyInCoordinateFrame(const float4x4& coordinateFrame) const;
    
    BoundingBox worldBoundingBox() const;
    BoundingBox worldBoundingBoxOfHierarchy() const;
    
    SDFOperation operation() const { return _operation; }
    void setOperation(SDFOperation);
    
    void setShouldChildrenShareId(bool should);
    
    void invalidate();
    
    // Editor bindings
    float translationX() const;
    void setTranslationX(float);

    float translationY() const;
    void setTranslationY(float);

    float translationZ() const;
    void setTranslationZ(float);
    
    float scale() const;
    void setScale(float);
    
private:
    
    void invalidateCachedWorldTransform();
    void _removeFromParent(bool invalidateWorldTransform);

    float _translation(size_t) const;
    void _setTranslation(size_t, float);
    
    const WorldWPtr _world;
    
    ObjectID _id = 0;
    Material3D::Ptr _material;
    SDFOperation _operation = SDFOperation::addition;
    
    Object3D::WPtr _parent;
    std::vector<Ptr> _children;
    
    float4x4 _localTransform = float4x4_identity();
    bool _selected = false;
    bool _shouldChildrenShareId = false;
    
    mutable float4x4 _cachedWorldTransform;
    mutable bool _cachedWorldTransformValid = false;
};

template <typename TGeometry>
class TObject3D : public Object3D
{
public:
    using _inherited = Object3D;
    
    TObject3D(const WorldPtr& world, const TGeometry& geometry)
    : _inherited(world), _geometry(geometry)
    {}
    
    const Type* geometryType() const override
    {
        return &TType<TGeometry>::instance();
    }
    
    void* geometry() override
    {
        return &_geometry;
    }
    
    bool isCulled(const float4x4& viewProjectionMatrix) const override
    {
        const auto worldViewProjMatrix = viewProjectionMatrix * worldTransform();
        const auto box = _geometry.boundingBox();
        
        return box.isCulled(worldViewProjMatrix);
    }
    
    BoundingBox localBoundingBox() const override
    {
        return _geometry.boundingBox();
    }
    
    void selfSerialize(SerializationContext& context) const final override
    {
        context.serializeObjectHeader([this](ObjectHeader* header)
        {
            RSTTransformer transformer { worldTransform() };
            
            SDFObject<TGeometry, RSTTransformer> object { _geometry, transformer };
            
            const bool selected = this->selected();
            if (selected)
            {
                object.setExtraCullingMargin(selected ? kOutlineThickness : 0.f);
            }
            
            const auto materialId = materialID();
         
            return serializeObject<SDFObject<TGeometry, RSTTransformer>>(
                                                header,
                                               object,
                                               id(),
                                                materialId,
                                               object.objectType(),
                                               transformer.transformerType(),
                                                operation(),
                                               selected);
        });
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

class WorldDelegate
{
public:
    using Ptr = std::shared_ptr<WorldDelegate>;
    using WPtr = std::weak_ptr<WorldDelegate>;
    virtual ~WorldDelegate() = default;
    
    virtual void onChange(const WorldPtr&) = 0;
    virtual void onSelectionChanged(const WorldPtr& world, const Object3D::Ptr& oldObject, const Object3D::Ptr& newObject) = 0;
    
};

class World final : public std::enable_shared_from_this<World>
{
public:
    static WorldPtr make();
    
    void serialize(const float4x4& viewProjectionMatrix, SerializedWorldObject&, Materials& materials) const;
    
    WorldDelegate::Ptr delegate() const;
    void setDelegate(const WorldDelegate::Ptr&);
    
    void addMaterial(const Material3D::Ptr&);
    Material3D::Ptr addMaterial(const float4& color);
    
    Object3D::Ptr rootObject() const { return _rootObject; }
    
    const Object3D::Ptr& selectedObject() const { return _selectedObject; }
    void setSelectedObject(const Object3D::Ptr&);
    
    CommandHistory& commandHistory() { return _commandHistory; }
    
    ObjectID generateNewObjectID();
    
    void invalidate();
    
private:
    World() = default;
    
    void init();
    
    Object3D::Ptr _rootObject;
    
    ObjectID _nextAvailableObjectID = 1;
    std::vector<Material3D::Ptr> _materials;
    
    Object3D::Ptr _selectedObject;
    
    CommandHistory _commandHistory;
    
    WorldDelegate::WPtr _delegate;
};
