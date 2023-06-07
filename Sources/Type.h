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

enum class PropertyType
{
    Float,
    Int
};

template <typename T>
PropertyType getPropertyType();

template <typename T>
class RangedValue final
{
    T value = {};
    T minValue = std::numeric_limits<T>::min();
    T maxValue = std::numeric_limits<T>::max();
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

using TPropertyRangedValue = std::variant<FloatRangedValue, IntRangedValue>;
using TPropertyValue = std::variant<float, int>;

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
             const TPropertyRangedValue& defaultValue = {});
    
    virtual ~Property() = default;
    
    TPropertyValue get(const void* object) const;
    
    const std::string& name() const { return _name; }
    
private:
    const std::string _name;
    const PropertyType _propertyType;
    const Getter _getter;
    const Setter _setter;
    const TPropertyRangedValue _defaultValue;
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
    
    const std::vector<Property::Ptr>& properties() const { return _properties; }
    
    TPropertyValue getPropertyValue(const void* object, const std::string& propName) const;
    
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

