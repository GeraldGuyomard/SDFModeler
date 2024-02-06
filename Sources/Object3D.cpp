//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Object3D.h"
#include "SerializationContext.h"
#include "RectF.h"

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

const Type*
Object3D::type() const
{
    return &TType<Object3D>::instance();
}

template <>
void initializeType<Object3D>(Type& type)
{
    {
        constexpr float minValue = -5.f;
        constexpr float maxValue = +5.f;
        
        type.addProperty<Object3D, float, &Object3D::translationX, &Object3D::setTranslationX>("x", minValue, maxValue);
        type.addProperty<Object3D, float, &Object3D::translationY, &Object3D::setTranslationY>("y", minValue, maxValue);
        type.addProperty<Object3D, float, &Object3D::translationZ, &Object3D::setTranslationZ>("z", minValue, maxValue);
    }
    
    {
        constexpr float minValue = -180.f;
        constexpr float maxValue = +180.f;
        
        type.addProperty<Object3D, float, &Object3D::rotationX, &Object3D::setRotationX>("rot x", minValue, maxValue);
        type.addProperty<Object3D, float, &Object3D::rotationY, &Object3D::setRotationY>("rot y", minValue, maxValue);
        type.addProperty<Object3D, float, &Object3D::rotationZ, &Object3D::setRotationZ>("rot z", minValue, maxValue);
    }
    
    type.addProperty<Object3D, float, &Object3D::scale, &Object3D::setScale>("scale");
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
    
    invalidate();
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
    invalidate();
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
    assert(isValid(transform));
    
    _localTransform = transform;
    invalidateCachedWorldTransform();
    invalidate();
}

float
Object3D::scale() const
{
    RSTTransformer t { _localTransform };
    return t.scale();
}

void
Object3D::setScale(float s)
{
    RSTTransformer t { _localTransform };
    t.setScale(s);
    
    setLocalTransform(t.transform());
}

float
Object3D::_translation(size_t index) const
{
    RSTTransformer t { _localTransform };
    return t.translation()[index];
}

void
Object3D::_setTranslation(size_t index, float v)
{
    RSTTransformer transform { _localTransform };
    
    float3 translation = transform.translation();
    translation[index] = v;
    transform.setTranslation(translation);
    
    setLocalTransform(transform.transform());
}

float
Object3D::_rotation(size_t index) const
{
    RSTTransformer t { _localTransform };
    return t.rotationEulers()[index];
}

void
Object3D::_setRotation(size_t index, float v)
{
    RSTTransformer transform { _localTransform };
    
    float3 rot = transform.rotationEulers();
    rot[index] = v;
    transform.setRotationEulers(rot);
    
    setLocalTransform(transform.transform());
}

float
Object3D::translationX() const
{
    return _translation(0);
}

void
Object3D::setTranslationX(float x)
{
    _setTranslation(0, x);
}

float
Object3D::translationY() const
{
    return _translation(1);
}

void
Object3D::setTranslationY(float x)
{
    _setTranslation(1, x);
}

float
Object3D::translationZ() const
{
    return _translation(2);
}

void
Object3D::setTranslationZ(float x)
{
    _setTranslation(2, x);
}

float
Object3D::rotationX() const
{
    return _rotation(0);
}

void
Object3D::setRotationX(float x)
{
    _setRotation(0, x);
}

float
Object3D::rotationY() const
{
    return _rotation(1);
}

void
Object3D::setRotationY(float y)
{
    _setRotation(1, y);
}

float
Object3D::rotationZ() const
{
    return _rotation(2);
}

void
Object3D::setRotationZ(float z)
{
    _setRotation(2, z);
}

void
Object3D::setLocalTransform(const RSTTransformer& transformer)
{
    setLocalTransform(transformer.transform());
}

void
Object3D::setOperation(SDFOperation op)
{
    _operation = op;
}

TileDescriptor::TileDescriptor(Tile& tile)
: tile(tile), tileRect(tile.minPt, tile.maxPt)
{}

bool
Object3D::serializeHierarchy(TileDescriptor& tileDescriptor, SerializationContext& context) const
{
    const bool thisVisible = !context.isCulled(*this, tileDescriptor.tileRect);
    if (thisVisible)
    {
        selfSerialize(tileDescriptor, context);
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
        if (child->serializeHierarchy(tileDescriptor, context))
        {
            positiveChildVisible = true;
        }
    }
    
    if (positiveChildVisible)
    {
        for (const auto& child : negativeChildren)
        {
            if (!context.isCulled(*child, tileDescriptor.tileRect))
            {
                child->serializeHierarchy(tileDescriptor, context);
            }
        }
    }
    
    return thisVisible;
}

void
Object3D::selfSerialize(TileDescriptor&, SerializationContext&) const
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
World::serialize(SerializationContext& context,
                 Materials& materials) const
{
    auto& serialized = context.serializedWorldObject();
    
    const size_t nbTiles = serialized.numTileRows * serialized.numTileColumns;
    
    for (size_t index=0; index < nbTiles; ++index)
    {
        auto& tile = serialized.tiles[index];
        tile.offsetInBuffer = context.currentAvailableOffsetInBuffer();
        
        TileDescriptor descr { tile };
        _rootObject->serializeHierarchy(descr, context);
    }
    
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
        const auto old = _selectedObject;
        
        if (old != nullptr)
        {
            old->setSelected(false);
        }
        
        _selectedObject = object;
        
        if (_selectedObject != nullptr)
        {
            _selectedObject->setSelected(true);
        }
        
        if (auto delegate = this->delegate())
        {
            delegate->onSelectionChanged(shared_from_this(), old, _selectedObject);
        }
    }
}

WorldDelegate::Ptr
World::delegate() const
{
    return _delegate.lock();
}

void
World::setDelegate(const WorldDelegate::Ptr& delegate)
{
    _delegate = delegate;
}


void
World::invalidate()
{
    if (auto delegate = this->delegate())
    {
        delegate->onChange(shared_from_this());
    }
}
