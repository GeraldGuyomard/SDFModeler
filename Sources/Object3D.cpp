//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Object3D.h"
#include "SerializationContext.h"

Material3D::Material3D(const SimpleMaterial& m)
: _material(m)
{}

void
Material3D::setId(MaterialID id)
{
    _id = id;
}

Object3D::Object3D(const WorldPtr& world)
: _world(world)
{}

void
Object3D::setId(ObjectID id)
{
    _id = id;
}

Object3D::Ptr
Object3D::objectByID(ObjectID id) const
{
    if (id == _id)
    {
        return ((Object3D*) this)->shared_from_this();
    }
    
    for (const auto& object : children())
    {
        if (object->id() == id)
        {
            return object;
        }
    }
    
    return nullptr;
}

void
Object3D::setSelected(bool selected)
{
    if (_selected != selected)
    {
        _selected = selected;
        
        for (const auto& child : _children)
        {
            child->setSelected(selected);
        }
        
        invalidate();
    }
}

Object3DFactory::Object3DFactory(const std::string& name)
: _name(name)
{}

class Object3DFactoryRegistry final
{
public:
    static Object3DFactoryRegistry& instance()
    {
        static Object3DFactoryRegistry s_Instance;
        return s_Instance;
    }

    void addFactory(const Object3DFactory::Ptr& factory)
    {
        _factories.push_back(factory);
    }
    
    const std::vector<Object3DFactory::Ptr>& factories() const
    {
        return _factories;
    }
    
private:
    Object3DFactoryRegistry() = default;
    Object3DFactoryRegistry(const Object3DFactoryRegistry&) = delete;
    
    std::vector<Object3DFactory::Ptr> _factories;
};

void
Object3DFactory::addFactory(const Ptr& factory)
{
    Object3DFactoryRegistry::instance().addFactory(factory);
}

const std::vector<Object3DFactory::Ptr>&
Object3DFactory::factories()
{
    return Object3DFactoryRegistry::instance().factories();
}


void
Object3D::setMaterial(const Material3D::Ptr& mat)
{
    if (_material != mat)
    {
        _material = mat;
    }
}

ObjectID
Object3D::id() const
{
    const Object3D* object = this;
    while (object != nullptr)
    {
        if (object->_id != kInvalidObjectID)
        {
            return object->_id;
        }
        
        object = parent().get();
    }
    
    return kInvalidObjectID;
}

MaterialID
Object3D::materialID() const
{
    const Object3D* object = this;
    while (object != nullptr)
    {
        auto mat = object->_material;
        if (mat != nullptr)
        {
            return mat->id();
        }
        
        object = parent().get();
    }
    
    return kNoMaterialID;
}

void
Object3D::addChild(const Ptr& child)
{
    if (_shouldChildrenShareId)
    {
        child->_id = kInvalidObjectID;
    }
    else if (child->_id == kInvalidObjectID)
    {
        child->_id = world()->generateNewObjectID();
    }
    
    auto self = shared_from_this();
    
    if (child->parent() != self)
    {
        child->_removeFromParent(false);
        _children.push_back(child);
        child->_parent = self;
        
        child->invalidateCachedWorldTransform();
    }
}

void
Object3D::_removeFromParent(bool invalidateWorldTransform)
{
    if (auto parent = this->parent())
    {
        auto self = shared_from_this();
        _parent.reset();
        
        const auto end = parent->_children.end();
        for (auto it = parent->_children.begin(); it != end; ++it)
        {
            auto child = *it;
            if (self == child)
            {
                parent->_children.erase(it);
                
                if (invalidateWorldTransform)
                {
                    invalidateCachedWorldTransform();
                }
                
                return;
            }
        }
        
        assert(false);
    }
}

void
Object3D::removeFromParent()
{
    _removeFromParent(true);
}

void
Object3D::invalidateCachedWorldTransform()
{
    _cachedWorldTransformValid = false;
    
    for (const auto& child : children())
    {
        child->invalidateCachedWorldTransform();
    }
}

float4x4
Object3D::worldTransform() const
{
    if (!_cachedWorldTransformValid)
    {
        if (auto parent = this->parent())
        {
            _cachedWorldTransform = parent->worldTransform() * localTransform();
        }
        else
        {
            _cachedWorldTransform = localTransform();
        }
        
        _cachedWorldTransformValid = true;
    }
    
    return _cachedWorldTransform;
}

void
Object3D::setWorldTransform(const float4x4& transform)
{
    if (auto parent = this->parent())
    {
        const auto localTransform = inverse(parent->worldTransform()) * transform;
        setLocalTransform(localTransform);
    }
    else
    {
        return setLocalTransform(transform);
    }
}

BoundingBox
Object3D::boundingBoxInCoordinateFrame(const float4x4& coordinateFrame) const
{
    const auto localBox = localBoundingBox();
    if (localBox.empty())
    {
        return {};
    }
    
    const auto m = coordinateFrame * worldTransform();
    return m * localBox;
}

BoundingBox
Object3D::boundingBoxOfHierarchyInCoordinateFrame(const float4x4& coordinateFrame) const
{
    auto box = boundingBoxInCoordinateFrame(coordinateFrame);
    
    for (const auto& child : children())
    {
        const auto b = child->boundingBoxOfHierarchyInCoordinateFrame(coordinateFrame);
        box.add(b);
    }
    
    return box;
}

BoundingBox
Object3D::worldBoundingBox() const
{
    return boundingBoxInCoordinateFrame(float4x4_identity());
}

BoundingBox
Object3D::worldBoundingBoxOfHierarchy() const
{
    return boundingBoxOfHierarchyInCoordinateFrame(float4x4_identity());
}

void
Object3D::setLocalTransform(const float4x4& transform)
{
    _localTransform = transform;
    invalidateCachedWorldTransform();
    invalidate();
}

void
Object3D::setLocalTransform(const RSTTransformer& transformer)
{
    _localTransform = transformer.transform();
}

void
Object3D::setOperation(SDFOperation op)
{
    _operation = op;
}

bool
Object3D::serializeHierarchy(SerializationContext& context) const
{
    const bool thisVisible = !isCulled(context.viewProjectionMatrix());
    if (thisVisible)
    {
        selfSerialize(context);
    }
    
    std::vector<Ptr> positiveChildren;
    std::vector<Ptr> negativeChildren;

    for (const auto& child : children())
    {
        switch (child->operation())
        {
            case SDFOperation::addition:
            {
                positiveChildren.push_back(child);
                break;
            }

            case SDFOperation::substraction:
            {
                negativeChildren.push_back(child);
                break;
            }
                
            default: break;
        }
    }
    
    bool positiveChildVisible = false;
    for (const auto& child : positiveChildren)
    {
        if (child->serializeHierarchy(context))
        {
            positiveChildVisible = true;
        }
    }
    
    if (positiveChildVisible)
    {
        for (const auto& child : negativeChildren)
        {
            if (!child->isCulled(context.viewProjectionMatrix()))
            {
                child->serializeHierarchy(context);
            }
        }
    }
    
    return thisVisible;
}

void
Object3D::selfSerialize(SerializationContext&) const
{
}

void
Object3D::setShouldChildrenShareId(bool should)
{
    _shouldChildrenShareId = should;
}

void
Object3D::invalidate()
{
    if (auto world = this->world())
    {
        world->invalidate();
    }
}

WorldPtr
World::make()
{
    WorldPtr world(new World);
    world->init();
    return world;
}

void
World::init()
{
    _rootObject = std::make_shared<Object3D>(shared_from_this());
    _rootObject->setId(generateNewObjectID());
    
    addMaterial(float4 {1, 0, 0, 1});
}

void
World::serialize(const float4x4& viewProjectionMatrix, SerializedWorldObject& serializedWorld, Materials& materials) const
{
    SerializationContext context { serializedWorld, viewProjectionMatrix };
    _rootObject->serializeHierarchy(context);
    
    materials.nbMaterials = _materials.size();
    
    for (const auto& mat : _materials)
    {
        materials.material[mat->id()] = mat->material();
    }
}

ObjectID
World::generateNewObjectID()
{
    return _nextAvailableObjectID++;
}

void
World::addMaterial(const Material3D::Ptr& mat)
{
    mat->setId(_materials.size());
    _materials.push_back(mat);
}

Material3D::Ptr
World::addMaterial(const float4& color)
{
    auto mat = std::make_shared<Material3D>(SimpleMaterial { color });
    addMaterial(mat);
    return mat;
}

void
World::setSelectedObject(const Object3D::Ptr& object)
{
    if (_selectedObject != object)
    {
        if (_selectedObject != nullptr)
        {
            _selectedObject->setSelected(false);
        }
        
        _selectedObject = object;
        
        if (_selectedObject != nullptr)
        {
            _selectedObject->setSelected(true);
        }
        
    }
}

void
World::setInvalidationCallback(const InvalidationCallback& callback)
{
    _invalidationCallback = callback;
}

void
World::invalidate()
{
    if (_invalidationCallback != nullptr)
    {
        _invalidationCallback(shared_from_this());
    }
}
