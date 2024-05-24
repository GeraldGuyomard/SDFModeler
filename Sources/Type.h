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

using TDefaultPropertyValue = std::variant<FloatRangedValue, IntRangedValue, std::string>;
using TPropertyValue = std::variant<float, int, std::string>;

bool write(rapidjsonStringWriter& writer, const TPropertyValue& value);

class Type;

template <typename T>
const Type& getType();

class Value
{
public:
    virtual ~Value() = default;
    
    virtual const Type& type() const = 0;
    virtual void* value() const = 0;
};

class Property final
{
public:
    using Ptr = std::shared_ptr<Property>;
    
    using Getter = std::function<TPropertyValue (const void* object)>;
    using Setter = std::function<void (void* object, const TPropertyValue&)>;
    
    Property(const std::string& name,
             const Type& propType,
             const Getter& getter,
             const Setter& setter,
             const TDefaultPropertyValue& defaultValue = {});
    
    TPropertyValue get(const void* object) const;
    const TDefaultPropertyValue& defaultValue() const { return _defaultValue; }
    
    const std::string& name() const { return _name; }
    
    void set(void* object, const TPropertyValue&) const;
    
private:
    const std::string _name;
    const Type& _type;
    const Getter _getter;
    const Setter _setter;
    const TDefaultPropertyValue _defaultValue;
};

class Type
{
public:
    
    static const Type* typeByName(const std::string&);
    
    Type(const std::string& name, const Type* superType = nullptr);
    
    const std::string& name() const { return _name; }
    const Type* superType() const { return _superType; }
    
    template <typename TObject, typename TPropertyType>
    void addProperty(const std::string& name,
                     const std::function<TPropertyType (const TObject* object)>& getter,
                     const std::function<void (TObject* object, const TPropertyType& value)>& setter)
    {
        auto prop = std::make_shared<Property>(name, getPropertyType<TPropertyType>(),
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
        
        auto prop = std::make_shared<Property>(name, getType<std::string>(),
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
        
        auto prop = std::make_shared<Property>(name, getType<T>(),
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
    const std::vector<Property::Ptr>& allProperties() const;
    
    TPropertyValue getPropertyValue(const void* object, const std::string& propName) const;
    
    // serialization
    bool serialize(rapidjsonStringWriter& writer, const void* instance) const;
    
private:
    bool _serializeSelfProperties(rapidjson::Writer<rapidjson::StringBuffer>& writer, const void* instance) const;
    bool _serializeProperties(rapidjson::Writer<rapidjson::StringBuffer>& writer, const void* instance) const;
    
    const std::string _name;
    std::vector<Property::Ptr> _properties;
    mutable std::optional<std::vector<Property::Ptr>> _allProperties;
    
    const Type* const _superType;
};

template <typename T>
void initializeType(Type&);

template <typename T>
concept HasSuperType = requires
{
    typename T::_inherited;
};

std::string demangle(const std::type_info&);

template <typename T>
class TType final : public Type
{
public:
    static const TType& instance()
    {
        static const TType* s_Instance = createInstance();
        return *s_Instance;
    }
    
private:
    
    TType(const std::string& name, const Type* superType)
    : Type(name, superType)
    {}
    
    template <HasSuperType U>
    static inline const Type* superType()
    {
        using SuperType = U::_inherited;
        return &TType<SuperType>::instance();
    }

    template <typename U>
    static inline const Type* superType()
    {
        return nullptr;
    }
    
    static TType* createInstance()
    {
        const auto* sType  = superType<T>();
        TType* t = new TType { demangle(typeid(T)), sType };
        initializeType<T>(*t);
        return t;
    }
};

template <typename T>
inline const Type& getType()
{
    return TType<T>::instance();
}
