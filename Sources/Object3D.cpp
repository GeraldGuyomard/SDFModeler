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

void
World::addMaterial(Material3D::Ptr mat)
{
    mat->material().setSelected(true);
    
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
