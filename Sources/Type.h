//
//  Type.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 5/1/23.
//

#pragma once

#include <typeinfo>
#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <functional>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using rapidjsonStringWriter = rapidjson::Writer<rapidjson::StringBuffer>;

enum class PropertyType
{
    Float,
    Int,
    String
};

template <typename T>
PropertyType getPropertyType();

template <typename T>
class RangedValue
{
public:
    T value = {};
    T minValue = std::numeric_limits<T>::min();
    T maxValue = std::numeric_limits<T>::max();
    
    RangedValue() = default;
    RangedValue(T value, T minValue, T maxValue)
    : value(value), minValue(minValue), maxValue(maxValue)
    {}
};

using FloatRangedValue = RangedValue<float>;
using IntRangedValue = RangedValue<int32_t>;

template <>
inline PropertyType getPropertyType<int>()
{
    return PropertyType::Int;
}

template <>
inline PropertyType getPropertyType<float>()
{
    return PropertyType::Float;
}

template <>
inline PropertyType getPropertyType<std::string>()
{
    return PropertyType::String;
}

using TDefaultPropertyValue = std::variant<FloatRangedValue, IntRangedValue, std::string>;
using TPropertyValue = std::variant<float, int, std::string>;

bool write(rapidjsonStringWriter& writer, const TPropertyValue& value);

class Property
{
public:
    using Ptr = std::unique_ptr<Property>;
    
    using Getter = std::function<TPropertyValue (const void* object)>;
    using Setter = std::function<void (void* object, const TPropertyValue&)>;
    
    Property(const std::string& name,
             PropertyType propType,
             const Getter& getter,
             const Setter& setter,
             const TDefaultPropertyValue& defaultValue = {});
    
    virtual ~Property() = default;
    
    TPropertyValue get(const void* object) const;
    const TDefaultPropertyValue& defaultValue() const { return _defaultValue; }
    
    const std::string& name() const { return _name; }
    
    void set(void* object, const TPropertyValue&) const;
    
private:
    const std::string _name;
    const PropertyType _propertyType;
    const Getter _getter;
    const Setter _setter;
    const TDefaultPropertyValue _defaultValue;
};

class Type
{
public:
    
    template <typename TObject, typename TPropertyType>
    void addProperty(const std::string& name,
                     const std::function<TPropertyType (const TObject* object)>& getter,
                     const std::function<void (TObject* object, const TPropertyType& value)>& setter)
    {
        auto prop = std::make_unique<Property>(name, getPropertyType<TPropertyType>(),
        [getter](const void* object) -> TPropertyValue
        {
            TObject* o = (TObject*) object;
            return { getter(o) };
        },
                                               
        [setter](void* object, const TPropertyValue& value)
        {
            TObject* o = (TObject*) object;
            
            if (auto v = std::get_if<TPropertyType>(&value))
            {
                setter(o, *v);
            }
        });
        
        
        _properties.push_back(std::move(prop));
    }
    
    template <class TObject, std::string (TObject::*TGet)() const, void (TObject::*TSet)(const std::string&)>
    void addStringProperty(const std::string& name, const std::string& defaultV = {})
    {
        const TDefaultPropertyValue defaultValue { defaultV };
        
        auto prop = std::make_unique<Property>(name, getPropertyType<std::string>(),
        [](const void* object) -> TPropertyValue
        {
            TObject* o = (TObject*) object;
            return { (o->*TGet)() };
        },
                                               
        [](void* object, const TPropertyValue& value)
        {
            TObject* o = (TObject*) object;
            
            if (auto v = std::get_if<std::string>(&value))
            {
                (o->*TSet)(*v);
            }
        }, defaultValue);
        
        _properties.push_back(std::move(prop));
    }
    
    template <class TObject, typename T, T (TObject::*TGet)() const, void (TObject::*TSet)(T)>
    void addProperty(const std::string& name, T minValue = 0 , T maxValue = 5)
    {
        const RangedValue<T> defaultValue { T{}, minValue, maxValue };
        
        auto prop = std::make_unique<Property>(name, getPropertyType<T>(),
        [](const void* object) -> TPropertyValue
        {
            TObject* o = (TObject*) object;
            return { (o->*TGet)() };
        },
                                               
        [](void* object, const TPropertyValue& value)
        {
            TObject* o = (TObject*) object;
            
            if (auto v = std::get_if<T>(&value))
            {
                (o->*TSet)(*v);
            }
        }, defaultValue);
        
        _properties.push_back(std::move(prop));
    }
    
    const std::vector<Property::Ptr>& properties() const { return _properties; }
    
    TPropertyValue getPropertyValue(const void* object, const std::string& propName) const;
    
    // serialization
    bool serialize(rapidjsonStringWriter& writer, const void* instance) const;
    
private:
    std::vector<Property::Ptr> _properties;
};

template <typename T>
void initializeType(Type&);

template <typename T>
class TType : public Type
{
public:
    static const TType& instance()
    {
        static const TType* s_Instance = createInstance();
        return *s_Instance;
    }
    
private:
    static TType* createInstance()
    {
        TType* t = new TType;
        initializeType<T>(*t);
        return t;
    }
};

