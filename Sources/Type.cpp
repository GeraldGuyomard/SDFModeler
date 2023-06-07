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
                   const TPropertyRangedValue& defaultValue)
: _name(name), _propertyType(propType), _getter(getter), _setter(setter), _defaultValue(defaultValue)
{}

TPropertyValue
Property::get(const void* object) const
{
    return _getter(object);
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
