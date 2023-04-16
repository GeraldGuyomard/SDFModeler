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

MaterialID
Object3D::materialID() const
{
    return (_material != nullptr) ? _material->id() : kNoMaterialID;
}

void
Object3D::addChild(const Ptr& child)
{
    if (child->_id == kInvalidObjectID)
    {
        child->_id = world()->generateNewObjectID();
    }
    
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
                return;
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
Object3D::setLocalTransform(const float4x4& transform)
{
    _localTransform = transform;
}

void
Object3D::setLocalTransform(const RSTTransformer& transformer)
{
    _localTransform = transformer.transform();
}

void
Object3D::setOperation(Operation op)
{
    _operation = op;
}

void
Object3D::serializeHierarchy(SerializedWorld& serializedWorld, uint8_t*& p) const
{
    const size_t size = selfSerialize(p);
    if (size != 0)
    {
        p += size;
        
        const auto geomIndex = serializedWorld.geometriesCount;
        serializedWorld.geometriesCount++;
        
        ObjectHeader objectHeader {id(), uint32_t(materialID()), selected() };
        serializedWorld.simpleObjectHeaders[serializedWorld.simpleObjectsCount] = { objectHeader, geomIndex  };
        
        serializedWorld.simpleObjectsCount++;
    }
    
    for (const auto& child : children())
    {
        child->serializeHierarchy(serializedWorld, p);
    }
}

void
Object3D::serialize(SerializedWorld& serializedWorld) const
{
    serializedWorld.geometriesCount = 0;
    serializedWorld.simpleObjectsCount = 0;
    serializedWorld.compoundObjectsCount = 0;
    
    uint8_t* p = reinterpret_cast<uint8_t*>(&(serializedWorld.geometries));
    
    serializeHierarchy(serializedWorld, p);
}

size_t
Object3D::selfSerialize(uint8_t* ptr) const
{
    return 0;
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
World::serialize(SerializedWorld& serializedWorld, Materials& materials) const
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
