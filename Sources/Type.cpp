//
//  Type.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 5/1/23.
//

#include "Type.h"
#include <cxxabi.h>

std::string demangle(const std::type_info& info)
{
    const char* abiName = info.name();
    
    char name[256];
    size_t size = sizeof(name);
    int status;
    abi::__cxa_demangle(abiName, name, &size, &status);
    
    return { name };
}

Type::Type(const std::string& name, const Type* superType)
: _name(name), _superType(superType)
{}

Property::Property(const std::string& name, const Type& type,
                   const Getter& getter,
                   const Setter& setter,
                   const TDefaultPropertyValue& defaultValue)
: _name(name), _type(type), _getter(getter), _setter(setter), _defaultValue(defaultValue)
{}

namespace
{
    void recurseProps(const Type* t, std::vector<Property::Ptr>& oProps)
    {
        if (t != nullptr)
        {
            recurseProps(t->superType(), oProps);
            
            const auto& p = t->properties();
            oProps.insert(oProps.end(), p.begin(), p.end());
        }
    }
}

const std::vector<Property::Ptr>& Type::allProperties() const
{
    if (!_allProperties.has_value())
    {
        std::vector<Property::Ptr> p;
        recurseProps(this, p);
        _allProperties = p;
    }
    
    return _allProperties.value();
}

TPropertyValue
Property::get(const void* object) const
{
    return _getter(object);
}

void
Property::set(void* object, const TPropertyValue& v) const
{
    _setter(object, v);
}

TPropertyValue
Type::getPropertyValue(const void* object, const std::string& propName) const
{
    for (const auto& prop : _properties)
    {
        if (prop->name() == propName)
        {
            return prop->get(object);
        }
    }
    
    return {};
}

bool write(rapidjsonStringWriter& writer, const TPropertyValue& value)
{
    if (const int* v = std::get_if<int>(&value))
    {
        writer.Int(*v);
        return true;
    }
    else if (const float* v = std::get_if<float>(&value))
    {
        writer.Double(*v);
        return true;
    }
    else if (const std::string* v = std::get_if<std::string>(&value))
    {
        writer.String(v->c_str());
        return true;
    }
    else
    {
        assert(false);
        return false;
    }
}

bool Type::_serializeSelfProperties(rapidjson::Writer<rapidjson::StringBuffer>& writer, const void* instance) const
{
    for (const auto& prop : _properties)
    {
        writer.Key(prop->name().c_str());
       
        const auto val = prop->get(instance);
        if (!write(writer, val))
        {
            return false;
        }
    }
    
    return true;
}

bool Type::_serializeProperties(rapidjson::Writer<rapidjson::StringBuffer>& writer, const void* instance) const
{
    if (auto sType = superType())
    {
        if (!sType->_serializeProperties(writer, instance))
        {
            return false;
        }
    }
    
    return _serializeSelfProperties(writer, instance);;
}

bool Type::serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const void* instance) const
{
    writer.Key("type");
    writer.String(_name.c_str());
    
    writer.Key("properties");
    writer.StartObject();
    
    if (!_serializeProperties(writer, instance))
    {
        return false;
    }
    
    writer.EndObject();
    
    return true;
}

template <>
void initializeType<std::string>(Type&)
{
    
}

template <>
void initializeType<float>(Type&)
{
    
}
