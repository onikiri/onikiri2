// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>
#include "Sim/Foundation/Resource/Builder/ResourceFactory.h"

using namespace Onikiri;


//
//
//
ResourceFactory::ResourceFactory()
{
    InitializeResourceMap();
}

//
//
//
template <typename T>
class ResourceInterfaceTrait : public ResourceTypeTraitBase
{
public:
    ResourceInterfaceTrait()
    {
    }

    virtual PhysicalResourceNode* CreateInstance()
    {
        return NULL;
    }

    virtual void* DynamicCast(PhysicalResourceIF* ptr)
    {
        return dynamic_cast<T*>(ptr);
    }
};

template <typename T>
class ResourceTypeTrait : public ResourceTypeTraitBase
{
public:
    ResourceTypeTrait()
    {
    }

    virtual PhysicalResourceNode* CreateInstance()
    {
        return new T();
    }

    virtual void* DynamicCast(PhysicalResourceIF* ptr)
    {
        return dynamic_cast<T*>(ptr);
    }
};

//
// Check whether the type name is registered or not.
//
void ResourceFactory::CheckTypeRegistered(const String& typeName)
{
    MapType::iterator i = m_resTypeMap.find( typeName );
    if( i != m_resTypeMap.end() ){
        THROW_RUNTIME_ERROR(
            "'%s' is already registered in 'ResourceFactory'.",
            typeName.c_str()
        );
    }
}

//
// Get a type trait object specified by the type name
//
ResourceTypeTraitBase* ResourceFactory::GetTrait( const String& typeName )
{
    MapType::iterator i = m_resTypeMap.find( typeName );
    if(i == m_resTypeMap.end()){
        THROW_RUNTIME_ERROR(
            "The resource type '%s' is not registered in the map of 'ResourceFactory'.", 
            typeName.c_str()
        );
    }
    return i->second;
}

//
// Create a instance specified by the passed name.
//
PhysicalResourceNode* ResourceFactory::CreateInstance(const String& typeName)
{
    PhysicalResourceNode* instance = 
        GetTrait( typeName )->CreateInstance();

    if( !instance ){
        THROW_RUNTIME_ERROR(
            "Could not create the instance of the '%s'.\n"
            "This class is an interface.",
            typeName.c_str()
        );
    }

    instance->SetTypeConverter( this );
    return instance;
}

//
// 
//
void* ResourceFactory::DynamicCast(const String& typeName, PhysicalResourceIF* orgPtr)
{
    return GetTrait( typeName )->DynamicCast( orgPtr );
}


//
// --- Macros for type map
//
#define BEGIN_RESOURCE_TYPE_MAP() \
    void ResourceFactory::InitializeResourceMap(){

#define END_RESOURCE_TYPE_MAP() \
        InitializeUserResourceMap(); \
    }

#define BEGIN_USER_RESOURCE_TYPE_MAP() \
    void ResourceFactory::InitializeUserResourceMap(){

#define END_USER_RESOURCE_TYPE_MAP() \
    }

#define RESOURCE_TYPE_ENTRY( typeName ) \
    CheckTypeRegistered( #typeName ); \
    m_resTypeMap[ #typeName ] = new ResourceTypeTrait<typeName>();

#define RESOURCE_INTERFACE_ENTRY( typeName ) \
    CheckTypeRegistered( #typeName ); \
    m_resTypeMap[ #typeName ] = new ResourceInterfaceTrait<typeName>();

#include "Sim/ResourceMap.h"
#include "User/UserResourceMap.h"
