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
Object3DCollection::addObject(Object3D::Ptr object)
{
    _objects.push_back(std::move(object));
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
World::addObject(Object3D::Ptr object)
{
    object->setId(_nextAvailableObjectID++);
    _content.addObject(std::move(object));
}

const std::vector<Object3D::Ptr>&
World::objects() const
{
    return _content.objects();
}

void
World::addMaterial(Material3D::Ptr mat)
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
World::setSelection(const Selection& selection)
{
    _selection = selection;
    
    const auto ids = _selection.objectIDs();
    
    for (const auto& object : objects())
    {
        const bool selected = (ids.find(object->id()) != ids.end());
        object->setSelected(selected);
    }
}
