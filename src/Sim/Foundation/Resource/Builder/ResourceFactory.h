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


#ifndef __RESOURCE_MAP_H
#define __RESOURCE_MAP_H

#include "Interface/ResourceIF.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri
{
    // ---
    class ResourceTypeTraitBase
    {
    public:
        virtual ~ResourceTypeTraitBase(){};
        virtual PhysicalResourceNode* CreateInstance() = 0;
        virtual void* DynamicCast( PhysicalResourceIF* ) = 0;
    };


    class ResourceFactory : public ResourceTypeConverterIF
    {
        typedef std::map< String, ResourceTypeTraitBase* > MapType;
        MapType m_resTypeMap;

        void CheckTypeRegistered(const String& typeName);
        ResourceTypeTraitBase* GetTrait( const String& typeName );
        void InitializeUserResourceMap();
    public:

        ResourceFactory();
        void InitializeResourceMap();
        PhysicalResourceNode* CreateInstance(const String& typeName);
        void* DynamicCast(const String& typeName, PhysicalResourceIF* orgPtr);
    };
}

#endif

