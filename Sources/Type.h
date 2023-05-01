//
//  Type.hpp
//  SDFModeler
//
//  Created by Gérald Guyomard on 5/1/23.
//

#pragma once

class Type
{
public:
    
private:
    
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
