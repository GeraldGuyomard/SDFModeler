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

class Property
{
public:
    using Ptr = std::unique_ptr<Property>;
    
    virtual ~Property() = default;
    
    virtual const std::type_info& typeInfo() const = 0;
};

template <typename T>
class TProperty : public Property
{
public:
    const std::type_info& typeInfo() const override
    {
        return typeid(T);
    }
};

class Type
{
public:
    
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
