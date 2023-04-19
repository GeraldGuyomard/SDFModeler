//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Object3D.h"

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

void
Object3D::setLocalTransform(const float4x4& transform)
{
    _localTransform = transform;
    invalidateCachedWorldTransform();
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

void
Object3D::serializeHierarchy(SerializedWorldObject& serializedWorld, uint8_t*& p) const
{
    const size_t size = selfSerialize(p);
    if (size != 0)
    {
        p += size;
        serializedWorld.objectCount++;
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
    
    if (!positiveChildren.empty())
    {
        for (const auto& child : positiveChildren)
        {
            child->serializeHierarchy(serializedWorld, p);
        }
        
        for (const auto& child : negativeChildren)
        {
            child->serializeHierarchy(serializedWorld, p);
        }
    }
}

void
Object3D::serialize(SerializedWorldObject& serializedWorld) const
{
    serializedWorld.objectCount = 0;
    
    uint8_t* p = reinterpret_cast<uint8_t*>(&(serializedWorld.buffer));
    
    serializeHierarchy(serializedWorld, p);
}

size_t
Object3D::selfSerialize(uint8_t* ptr) const
{
    return 0;
}

void
Object3D::setShouldChildrenShareId(bool should)
{
    _shouldChildrenShareId = should;
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
World::serialize(SerializedWorldObject& serializedWorld, Materials& materials) const
{
    _rootObject->serialize(serializedWorld);
    
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

