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

void
Object3D::setId(ObjectID id)
{
    _id = id;
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
        onMaterialChange();
    }
}

MaterialID
Object3D::materialID() const
{
    return (_material != nullptr) ? _material->id() : kNoMaterialID;
}

void
Object3D::addChild(const Ptr& child)
{
    auto self = shared_from_this();
    
    if (child->parent() != self)
    {
        child->removeFromParent();
        _children.push_back(child);
        child->_parent = self;
    }
}

void
Object3D::removeFromParent()
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
                break;
            }
        }
        
        assert(false);
    }
}


float4x4
Object3D::worldTransform() const
{
    if (auto parent = this->parent())
    {
        return parent->worldTransform() * localTransform();
    }
    else
    {
        return localTransform();
    }
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
Object3D::setOperation(Operation op)
{
    _operation = op;
}

void
Object3DCollection::addObject(const Object3D::Ptr& object)
{
    _objects.push_back(object);
}

void
Object3DCollection::removeObject(const Object3D::Ptr& object)
{
    const auto end = _objects.end();
    for (auto it = _objects.begin(); it != end; ++it)
    {
        if (*it == object)
        {
            _objects.erase(it);
            break;
        }
    }
}


std::set<ObjectID>
Object3DCollection::objectIDs() const
{
    std::set<ObjectID> ids;
    
    for (const auto& object : _objects)
    {
        ids.insert(object->id());
    }
    
    return ids;
}

Object3D::Ptr
Object3DCollection::anyObject() const
{
    return !_objects.empty() ? _objects.front() : nullptr;
}

bool
Object3DCollection::contains(const Object3D::Ptr& object) const
{
    for (const auto& o : _objects)
    {
        if (o == object)
        {
            return true;
        }
    }
    
    return false;
}

void
Object3DCollection::serialize(SerializedObjects& serializedObjects) const
{
    serializedObjects.objectCount = 0;
    
    uint8_t* p = reinterpret_cast<uint8_t*>(&(serializedObjects.buffer));
    
    for (const auto& object : _objects)
    {
        const size_t size = object->serialize(p);
        p += size;
        serializedObjects.objectCount++;
    }
}

World::World()
{
    addMaterial(float4 {1, 0, 0, 1});
}

void
World::serialize(SerializedWorld& serializedWorld, Materials& materials) const
{
    _content.serialize(serializedWorld.content);
    
    materials.nbMaterials = _materials.size();
    
    for (const auto& mat : _materials)
    {
        materials.material[mat->id()] = mat->material();
    }
}

void
World::addObject(const Object3D::Ptr& object)
{
    object->setId(_nextAvailableObjectID++);
    _content.addObject(object);
}

void
World::removeObject(const Object3D::Ptr& object)
{
    _content.removeObject(object);
}

const std::vector<Object3D::Ptr>&
World::objects() const
{
    return _content.objects();
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

Object3D::Ptr
World::objectByID(ObjectID id) const
{
    for (const auto& object : objects())
    {
        if (object->id() == id)
        {
            return object;
        }
    }
    return nullptr;
}

void
World::setSelectedObject(const Object3D::Ptr& object)
{
    _selectedObject = object;
    
    for (const auto& object : objects())
    {
        const bool selected = (object == _selectedObject);
        object->setSelected(selected);
    }
}
