//
//  Type.cpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 5/1/23.
//

#include "Type.h"

Property::Property(const std::string& name, PropertyType propType,
                   const Getter& getter,
                   const Setter& setter,
                   const TDefaultPropertyValue& defaultValue)
: _name(name), _propertyType(propType), _getter(getter), _setter(setter), _defaultValue(defaultValue)
{}

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

bool Type::serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const void* instance) const
{
    for (const auto& prop : properties())
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
