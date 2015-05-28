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
#include "Sim/Foundation/Resource/ResourceNode.h"

using namespace Onikiri;

PhysicalResourceNode::PhysicalResourceNode()
{
    m_typeConverter = NULL;
    m_rid = RID_INVALID;
    m_initialized = false;
    m_totalEntryCount = 0;
    m_connectedEntryCount = 0;
}

PhysicalResourceNode::~PhysicalResourceNode()
{
    if( !IsParameterReleased() ){
        THROW_RUNTIME_ERROR(
            "'ReleaseParam()' was not called in the node '%s'.\n"
            "A class inherited from the 'PhysicalResourceNode' "
            "must call 'ReleaseParam()' before its destruction."
            "This method call can not be automated, "
            "because the destructor of the 'PhysicalResourceNode' "
            "can not touch the member of the inherited class.",
            m_info.name.c_str()
        );
    }

}

void PhysicalResourceNode::ReleaseParam()
{
    PhysicalResourceNode::ProcessParamMap(true);
    ParamExchange::ReleaseParam();
}

void PhysicalResourceNode::SetInfo( const PhysicalResourceNodeInfo& info )
{
    m_initialized = true;
    m_info = info;
    if( info.name == "" ||
        info.typeName == "" ||
        info.paramPath.ToString() == "" ||
        info.resultPath.ToString() == "" ||
        info.resultRootPath.ToString() == ""
    ){
        THROW_RUNTIME_ERROR( "The passed info is invalid." );
    }
    m_who = GetName() + "(" + GetTypeName() + ")";
}

const PhysicalResourceNodeInfo& PhysicalResourceNode::GetInfo()
{
    return m_info;
}

const char* PhysicalResourceNode::Who() const
{
    return m_who.c_str();
}

const String& PhysicalResourceNode::GetName() const 
{
    return m_info.name;
}

const String& PhysicalResourceNode::GetTypeName() const 
{
    return m_info.typeName;
}

const ParamXMLPath& PhysicalResourceNode::GetParamPath() const 
{
    if( !m_initialized ){
        THROW_RUNTIME_ERROR(
            "This physical resource is not initialized. "
            "Does call LoadParam() in a constructor? "
            "LoadParam() must be called in or after INIT_PRE_CONNECTION phase."
        );
    }

    return m_info.paramPath;
}
    
const ParamXMLPath& PhysicalResourceNode::GetResultPath() const 
{
    if( !m_initialized ){
        THROW_RUNTIME_ERROR(
            "This physical resource is not initialized. "
            "This error here may simply mean LoadParam() is called in a constructor. "
            "LoadParam() must be called in or after INIT_PRE_CONNECTION phase."
        );
    }

    return m_info.resultPath;
}

const ParamXMLPath& PhysicalResourceNode::GetResultRootPath() const
{
    return m_info.resultRootPath;
}

void PhysicalResourceNode::SetTypeConverter( ResourceTypeConverterIF* converter )
{
    m_typeConverter = converter;
}

// Check whether the node(s) is connected correctly.
void PhysicalResourceNode::CheckConnection(
    PhysicalResourceBaseArray& srcArray, 
    const String& to, 
    const ResourceConnectionResult& result 
){
    if( srcArray.GetSize() == 0 ){
        THROW_RUNTIME_ERROR(
            "The passed resource array has no entry."
        );
    }

    PhysicalResourceNode* res = srcArray[0];

    if( result.connectedCount == 0 ){
        const char* srcName     = res->m_info.name.c_str();
        const char* toName      = (to == "") ? srcName : to.c_str();
        const char* nodeName    = m_info.name.c_str();
        THROW_RUNTIME_ERROR(
            "Could not connect '%s' to '%s' in '%s'. "
            "Does '%s' has a member '%s'?",
            srcName, toName, nodeName,
            nodeName, toName
        );
    }
    else if( result.connectedCount > 1 ){
        THROW_RUNTIME_ERROR(
            "The node '%s' is connected to the node '%s' more than once.\n"
            "Use 'Connection' and specify a name ('To' attribute) that the node is connected to.",
            res->m_info.name.c_str(),
            m_info.name.c_str()
        );
    }

    m_totalEntryCount = result.entryCount;
    m_connectedEntryCount++;
}

// Check the connected node is scalar.
void PhysicalResourceNode::CheckNodeIsScalar( 
    PhysicalResourceBaseArray& srcArray,
    const String& targetName,
    const String& orgName
){
    if( srcArray.GetSize() != 1 ){
        THROW_RUNTIME_ERROR( 
            "Cannot connect the array node '%s' to the scalar node '%s.%s'.\n"
            "Maybe you set a thread count that is less than a core count.\n"
            "A thread count is 'total' thread count in all cores, and "
            "a thread count is always bigger or equal to core count.",
            orgName.c_str(),
            m_info.name.c_str(),
            targetName.c_str()
        );
    }
}

// This method must be implemented in the BEGIN_RESOURCE_MAP macro.
ResourceConnectionResult PhysicalResourceNode::ConnectResource( 
    PhysicalResourceBaseArray& srcArray, 
    const String& orgName,
    const String& to, 
    bool chained
){
    // If this method was pure virtual function and BEGIN_RESOURCE_MAP was not defined,
    // an error message would be "ConnectResource is not defined in derived class".
    // This error message is not easy to understand the cause of the problem, 
    // thus use THROW_RUNTIME_ERROR and notify it.
    THROW_RUNTIME_ERROR(
        "Invalid connection.\nThe resource map is not defined in the '%s'.",
        m_info.typeName.c_str()
    );
    /*
    printf( 
    "'%s[%d]' : %s[%d] is connected.\n", 
    m_name.c_str(), m_rid,
    res->GetName().c_str(), res->GetRID()
    );
    */
    return ResourceConnectionResult();
}

// Validate connection.
void PhysicalResourceNode::ValidateConnection()
{
    if( m_connectedEntryCount < m_totalEntryCount ){
        const char* name = m_info.name.c_str();
        THROW_RUNTIME_ERROR( "There are resouce members that are not connected correctly in %s.", name );
    }
}
