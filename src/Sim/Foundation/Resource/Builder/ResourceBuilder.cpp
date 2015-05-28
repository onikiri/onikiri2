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


//
// The factory class for physical resources
//

#include <pch.h>
#include "Sim/Foundation/Resource/Builder/ResourceBuilder.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

static int INVALID_NODE_COUNT = -1;
static int INVALID_NODE_CLUSTER = -1;

static const char g_configNamePath[] = "/Session/Simulator/@Configuration";
static const char g_configRootPath[] = "/Session/Simulator/Configurations/";
static const char g_resultRootPath[] = "/Session/Result/Resource/";

ResourceBuilder::ResNode::ResNode()
{
    count = INVALID_NODE_COUNT;
    cluster = INVALID_NODE_CLUSTER;
    copyCount = INVALID_NODE_COUNT;
    external = false;
}

ResourceBuilder::ResourceBuilder()
{
    m_rootNode = ResNodePtr( new ResNode() );
}

ResourceBuilder::~ResourceBuilder()
{
    Release();
}

// Release all configuration data
void ResourceBuilder::Release()
{
    for( size_t i = 0; i < m_instanceList.size(); i++ ){
        m_instanceList[i]->Finalize();
    }

    m_rootNode->children.clear();
    m_constMap.clear();

    for( size_t i = 0; i < m_instanceList.size(); i++ ){
        delete m_instanceList[i];
    }
    m_instanceList.clear();
}

ParamXMLPath ResourceBuilder::GetConfigurationPath()
{
    // Get configuration name
    ParamXMLPath cfgNamePath = g_configNamePath;
    String cfgName;
    g_paramDB.Get( cfgNamePath, &cfgName );

    // Get configuration count
    ParamXMLPath cfgPath = g_configRootPath + cfgName;

    XMLNodeArray* copyNodes = 
        g_paramDB.GetXMLTree().GetNodeArray( cfgPath );

    if( copyNodes == NULL ){
        THROW_RUNTIME_ERROR(
            "The specified configuration '%s' (%s) is not found.",
            cfgName.c_str(),
            cfgPath.ToString().c_str()
        );
    }


    return cfgPath;
}

// Load the constant section in the parameter XML
void ResourceBuilder::LoadConstantSection( const ParamXMLPath& rootPath )
{
    ParamXMLPath constPath = rootPath + "Constant";
    ParamXMLTree& tree = g_paramDB.GetXMLTree();

    XMLNodePtr constantNode = 
        tree.GetNode( constPath );
    
    if( constantNode == NULL ){
        return;
    }

    XMLAttributeMap& attributes = 
        constantNode->attributes;

    for( XMLAttributeMap::iterator i = attributes.begin();
         i != attributes.end();
         ++i
     ){
         m_constMap[i->first] = 
             lexical_cast<int>( i->second->value );
         i->second->accessed = true;
    }

}

// Load the parameter section in the parameter XML.
// This method loads the path of the node and save it in the node map.
// The parameter value is loaded in a object inherited from the ParamExchange 
// with this saved path.
void ResourceBuilder::LoadParameterSection( const ParamXMLPath& rootPath )
{
    ParamXMLPath parameterBasePath = rootPath + "Parameter";

    ParamXMLTree& tree = 
        g_paramDB.GetXMLTree();

    XMLNodePtr xmlParent = tree.GetNode( parameterBasePath );
    if( xmlParent == NULL ){
        return;
    }

    for( XMLChildMap::iterator children = xmlParent->children.begin();
         children != xmlParent->children.end();
         ++children
    ){
        for( size_t childIndex = 0; childIndex < children->second.size(); childIndex++ ){
            XMLNodePtr xmlNode = children->second[childIndex];

            ParamXMLPath paramPath = parameterBasePath;
            paramPath.AddArray( xmlNode->name, (int)childIndex );
            const ParamXMLPath& namePath  = paramPath + "@Name";
            
            //const String& key = element;
            String name;
            bool hit = g_paramDB.Get( namePath.ToString(), &name, false );
            if( !hit ){
                THROW_RUNTIME_ERROR( 
                    "'%s' does not have 'Name' attribute.\n"
                    "A node in a 'Parameter' section must have 'Name' attribute.",
                    paramPath.ToString().c_str() 
                );
            }

            ResNodeMap::iterator entry = m_resNodeMap.find( name );
            if( entry == m_resNodeMap.end() ){
                THROW_RUNTIME_ERROR(
                    "The node of the name '%s' is not found in the 'Structure' section.\n"
                    "All nodes in a 'Parameter' section must be defined in a 'Structure' section.\n"
                    "path: %s",
                    name.c_str(),
                    paramPath.ToString().c_str() 
                );
            }

            entry->second->paramPath = paramPath.ToString();

            // Make result path
            ParamXMLPath resultPath = g_resultRootPath;
            resultPath.AddArray( xmlNode->name, (int)childIndex );
            entry->second->resultPath = resultPath.ToString();
        }
    }
}


//
// Get attribute of 'Structure' node 
//


int ResourceBuilder::GetStructureNodeValue(
    const XMLNodePtr node, const String& attrName )
{
    XMLAttributeMap::iterator attr = node->attributes.find( attrName );
    if( attr == node->attributes.end() ){
        return INVALID_NODE_COUNT;
    }

    const String& valueStr = attr->second->value;
    attr->second->accessed = true;

    map<String, int>::iterator i = m_constMap.find( valueStr );
    if( i != m_constMap.end() ){
        return i->second;
    }
    else{
        return lexical_cast<int>( valueStr );
    }
}

String ResourceBuilder::GetStructureNodeString(
    const XMLNodePtr node, const String& attrName )
{
    XMLAttributeMap::iterator attr = node->attributes.find( attrName );
    if( attr != node->attributes.end() ){
        attr->second->accessed = true;
        return attr->second->value;
    }
    else{
        return "";
    }
}

//
// --- Load the structure section
//

// Process structure nodes recursively
void ResourceBuilder::TraverseStructureNode(ResNodePtr resParent, XMLNodePtr xmlParent, int copyCount)
{
    for( XMLChildMap::iterator children = xmlParent->children.begin();
         children != xmlParent->children.end();
         ++children
    ){
        for( size_t childIndex = 0; childIndex < children->second.size(); childIndex++ ){
            XMLNodePtr xmlNode = children->second[childIndex];
            ResNodePtr resNode( new ResNode );

            const String& type = xmlNode->name;

            if( type == "Connection" ){
                // Connection node
                ResNode::Child child;
                child.name = GetStructureNodeString( xmlNode, "Name" );
                child.to   = GetStructureNodeString( xmlNode, "To" );
                resParent->children.push_back( child );
            }
            else{
                // General node
                resNode->type    = type;
                resNode->name    = GetStructureNodeString( xmlNode, "Name" );
                resNode->count   = GetStructureNodeValue( xmlNode, "Count" );
                resNode->cluster = GetStructureNodeValue( xmlNode, "Cluster" );

                if( resNode->count != INVALID_NODE_COUNT){
                    resNode->copyCount = copyCount;
                }

                //printf("type:%s\tname:%s\tcount:%d\n", node->type.c_str(), node->name.c_str(), node->count);

                String connectionTo = GetStructureNodeString( xmlNode, "To" );

                ResNodeMap::iterator entry = m_resNodeMap.find( resNode->name );
                if(entry != m_resNodeMap.end()){
                    // Update a existent node int the 'm_resNodeMap'
                    ResNodePtr entryPtr = entry->second;
                    
                    ResNode::Child child;
                    child.node = entryPtr;
                    child.to   = connectionTo;
                    resParent->children.push_back( child );

                    if( entryPtr->type != resNode->type ){
                        THROW_RUNTIME_ERROR(
                            "The node '%s' is re-defined by the different type '%s' in the 'Structure' section.\n"
                            "The original type of the node is '%s'.",
                            resNode->name.c_str(),
                            resNode->type.c_str(),
                            entryPtr->type.c_str()
                        );
                    }

                    if( entryPtr->count != resNode->count && 
                        entryPtr->count != INVALID_NODE_COUNT &&
                        resNode->count  != INVALID_NODE_COUNT 
                    ){
                        THROW_RUNTIME_ERROR(
                            "The count of '%s' is re-defined in 'Structure' section.",
                            resNode->name.c_str()
                        );
                    }

                    if( entryPtr->copyCount != resNode->copyCount && 
                        entryPtr->copyCount != INVALID_NODE_COUNT &&
                        resNode->copyCount != INVALID_NODE_COUNT 
                    ){
                        THROW_RUNTIME_ERROR(
                            "The copy count of '%s' is re-defined in 'Structure' section.",
                            resNode->name.c_str()
                        );
                    }

                    if( entryPtr->cluster != resNode->cluster && entryPtr->cluster != INVALID_NODE_CLUSTER ){
                        THROW_RUNTIME_ERROR(
                            "The cluster of '%s' is re-defined in 'Structure' section.",
                            resNode->name.c_str()
                        );
                    }

                    if( resNode->count != INVALID_NODE_COUNT ){
                        entryPtr->count = resNode->count;
                    }

                    if( resNode->copyCount != INVALID_NODE_COUNT ){
                        entryPtr->copyCount = resNode->copyCount;
                    }

                    if( resNode->cluster != INVALID_NODE_CLUSTER ){
                        entryPtr->cluster = resNode->cluster;
                    }

                    resNode = entryPtr;
                }
                else{   // if(entry != m_resNodeMap.end()){
                    
                    // Add a new node to the 'm_resNodeMap'
                    m_resNodeMap[resNode->name] = resNode;

                    ResNode::Child child;
                    child.node = resNode;
                    child.to   = connectionTo;
                    resParent->children.push_back( child );
                }
            }

            TraverseStructureNode( resNode, xmlNode, copyCount );
        }
    }
}


// Process structure nodes recursively
void ResourceBuilder::LoadStructureSection(const ParamXMLPath& structPath)
{
    const ParamXMLPath& copyPath = structPath + "/Structure/Copy";

    ParamXMLTree& tree = 
        g_paramDB.GetXMLTree();
    
    XMLNodeArray* copyNodes = 
        tree.GetNodeArray( copyPath );

    if( !copyNodes ){
        THROW_RUNTIME_ERROR(
            "The 'Structure' node '%s' does not have any copy node.",
            (structPath + "Structure").ToString().c_str()
        );
    }

    for( size_t i = 0; i < copyNodes->size(); i++ ){

        XMLNodePtr copyNode = copyNodes->at(i);
        int copyCount = GetStructureNodeValue( copyNode, "Count" );

        if( copyCount == INVALID_NODE_COUNT ){
            THROW_RUNTIME_ERROR( 
                "The 'Copy' node ('%s') does not have a 'Count' attribute.",
                copyPath.ToString().c_str()
            );
        }

        TraverseStructureNode( m_rootNode, copyNode, copyCount );
    }
}


// Construct each resource from the loaded node map
void ResourceBuilder::ConstructResources()
{
    vector<int> tidList;
    vector<int> pidList;

    for( ResNodeMap::iterator i = m_resNodeMap.begin();
         i != m_resNodeMap.end();
         ++i
    ){
        ResNodePtr node = i->second;
        int count = node->count;

        if( node->external )
            continue;

        for( int rid = 0; rid < count; rid++ ){
            // Create an instance of the resource
            PhysicalResourceNode* res = 
                m_factory.CreateInstance( node->type );
            PhysicalResourceNodeInfo info;

            info.name = node->name;
            info.typeName = node->type;
            info.paramPath = node->paramPath;
            info.resultRootPath = node->resultPath;
            info.resultPath.SetArray( node->resultPath, "Result", rid );

            if( info.paramPath.ToString() == "" ){
                THROW_RUNTIME_ERROR(
                    "The node '%s' does not have its parameter node in the parameter XML.\n"
                    "A node in the 'Structure' section must have its parameter node "
                    "in the 'Parameter' section.",
                    info.name.c_str()
                );
            }

            res->SetInfo( info );
            res->SetRID( rid );

            node->instances.push_back( res );
            m_instanceList.push_back( res );

            // Set TID
            // The count of 'Copy' is equal to the maximum count in the resources. 
            // Usually, 'Copy' is equal to the count of 'Thread'.
            int copyCount = node->copyCount;
            if( copyCount < count ){
                THROW_RUNTIME_ERROR( 
                    "The resource count  is greater than the copy count.\n"
                    "Node:%s Copy Count:%d  Resouce Count:%d",
                    info.name.c_str(),
                    copyCount,
                    count
                );
            }
            if( copyCount % count != 0 ){
                THROW_RUNTIME_ERROR( 
                    "The copy count is not divided by the resource count.\n"
                    "Copy Count:%d  Resouce Count:%d",
                    info.name.c_str(),
                    copyCount,
                    count
                );
            }
            int tidCount = copyCount / count;
            res->SetThreadCount( tidCount );
            for( int i = 0; i < tidCount; i++ ){
                res->SetTID( i, rid*tidCount+i );
            }

#if 0
            // Set TID
            CalculateID( tidList, rid, node->copyCount, node, CALC_ID_THREAD );
            res->SetThreadCount( tidList.size() );
            for( size_t i = 0; i < tidList.size(); i++ ){
                res->SetTID( i, tidList[i] );
            }

            // Set PID
            CalculateID( pidList, rid, m_processCount, node, CALC_ID_PROCESS );
            res->SetProcessCount( pidList.size() );
            for( size_t i = 0; i < pidList.size(); i++ ){
                res->SetPID( i, pidList[i] );
            }
#endif
        }
    }
}

// Connect each resource
void ResourceBuilder::ConnectResources()
{
    PhysicalResourceArray< PhysicalResourceNode > arrayArg;

    for( ResNodeMap::iterator i = m_resNodeMap.begin();
         i != m_resNodeMap.end();
         ++i
    ){
        vector< ResNode::Child >& children = i->second->children;
        vector< PhysicalResourceNode* >& instances = i->second->instances;

        for( vector< ResNode::Child >::iterator child = children.begin();
             child != children.end();
             ++child
        ){

            ResNodePtr childNode;
            if( child->node != NULL ){
                // The child is specified by the pointer.
                childNode = child->node;
            }
            else{
                // The child is specified by the name.
                ResNodeMap::iterator childNodeIterator =
                    m_resNodeMap.find( child->name );
                if( childNodeIterator == m_resNodeMap.end() ){
                    THROW_RUNTIME_ERROR(
                        "The node '%s' is not defined. "
                        "This node is in the '%s'.", 
                        child->name.c_str(),
                        i->second->name.c_str()
                    );
                }
                childNode = childNodeIterator->second;
            }

            vector< PhysicalResourceNode* >&
                childInstances = childNode->instances;

            if( instances.size() < childInstances.size() ){
                // A case that the number of the parents is smaller than that of children.
                ASSERT( childInstances.size() % instances.size() == 0 );
                for( size_t i = 0; i < instances.size(); i++ ){
                    
                    size_t childrenCount = childInstances.size() / instances.size();
                    arrayArg.Resize( (int)childrenCount );
                    for( size_t c = 0; c < childrenCount; c++ ){
                        arrayArg[(int)c] = childInstances[ i*childrenCount + c ];
                    }
                    instances[i]->ConnectResource( arrayArg, childNode->name, child->to, false );
                }
            }
            else{
                // A case that the number of the parents is equal or greater than that of children.
                // In this case, one child is shared by the multiple parents.
                ASSERT( instances.size() % childInstances.size() == 0 );
                for( size_t i = 0; i < instances.size(); i++ ){
                    arrayArg.Resize( 1 );
                    size_t childIndex = i*childInstances.size()/instances.size();
                    arrayArg[0] = childInstances[ childIndex ];
                    instances[i]->ConnectResource( arrayArg, childNode->name, child->to, false );
                }
            }
        }

    }
}

// Initialize all nodes
void ResourceBuilder::InitializeNodes( PhysicalResourceNode::InitPhase phase )
{
    for( ResNodeMap::iterator i = m_resNodeMap.begin();
         i != m_resNodeMap.end();
         ++i
    ){
        ResNodePtr node = i->second;
        for( int rid = 0; rid < node->count; rid++ ){
            node->instances[rid]->Initialize( phase );
        }
    }
}

// Validate connection.
void ResourceBuilder::ValidateConnection()
{
    for( ResNodeMap::iterator i = m_resNodeMap.begin();
         i != m_resNodeMap.end();
         ++i
    ){
        ResNodePtr node = i->second;
        for( int rid = 0; rid < node->count; rid++ ){
            node->instances[rid]->ValidateConnection();
        }
    }
}

// Set external resource
void ResourceBuilder::SetExternalResource( ResNode* node )
{
    ResNodePtr nodePtr = ResNodePtr( new ResNode() );
    *nodePtr = *node;
    nodePtr->external = true;
    m_resNodeMap[node->name] = nodePtr;
}

//
// --- Construct all physical resources
//
void ResourceBuilder::Construct()
{
    const ParamXMLPath& cfgPath = GetConfigurationPath();

    LoadConstantSection( cfgPath );
    LoadStructureSection( cfgPath );
    LoadParameterSection( cfgPath );

    //Dump();

    ConstructResources();
    InitializeNodes( PhysicalResourceNode::INIT_PRE_CONNECTION );
    ConnectResources();
    ValidateConnection();
    InitializeNodes( PhysicalResourceNode::INIT_POST_CONNECTION );

}

// --- Dump the loaded data for debug
void ResourceBuilder::Dump()
{
    printf("--- ResNode connection : \n");
    for( ResNodeMap::iterator i = m_resNodeMap.begin();
         i != m_resNodeMap.end();
         ++i
    ){
        printf( 
            "%s : %s : %d\n", 
            i->second->type.c_str(),
            i->second->name.c_str(),
            i->second->count 
        );

        for( size_t j = 0; j < i->second->children.size(); j++ ){
            ResNode::Child child = i->second->children[j];
            if( child.node != NULL ){
                ResNodePtr ptr = child.node;
                printf( 
                    "  %s : %s : %d\n", 
                    ptr->type.c_str(),
                    ptr->name.c_str(),
                    ptr->count 
                );
            }
            else{
                printf( 
                    "  Connection : %s : %s\n", 
                    child.name.c_str(),
                    child.to.c_str()
                );
            }
        }
    }

    printf("\n--- Constant list : \n");
    for( ConstantMap::iterator i = m_constMap.begin();
         i != m_constMap.end();
         ++i 
     ){
         printf( "%s : %d\n", i->first.c_str(), i->second );
    }

    printf("\n--- ResNode list : \n");
    for( ResNodeMap::iterator i = m_resNodeMap.begin();
         i != m_resNodeMap.end();
         ++i
    ){
        for( size_t j = 0; j < i->second->instances.size(); j++ ){
            PhysicalResourceNode* res = i->second->instances[j];
            printf( "%s tid(", i->second->name.c_str() );
            for( int ti = 0; ti < res->GetThreadCount(); ti++ ){
                printf( "%d", res->GetTID( ti ) );
                if( ti + 1 < res->GetThreadCount() )
                    printf(",");
            }
            printf( ")\n" );
/*          printf( ") pid(" );
            for( int pi = 0; pi < res->GetProcessCount(); pi++ ){
                printf( "%d", res->GetPID( pi ) );
                if( pi + 1 < res->GetProcessCount() )
                    printf(",");
            }
            printf( ")\n" );
*/      }
    }
}


