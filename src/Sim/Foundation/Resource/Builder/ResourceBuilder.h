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

#ifndef __RESOURCE_BUILDER_H
#define __RESOURCE_BUILDER_H

#include "Sim/Foundation/Resource/Builder/ResourceFactory.h"

namespace Onikiri
{
    class ResourceBuilder
    {
    public:
        // Node of structure
        struct ResNode
        {
            typedef 
                boost::shared_ptr<ResNode> 
                ResNodePtr;

            ResNodePtr parent;
            std::vector< PhysicalResourceNode* > instances;

            struct Child
            {
                String to;
                String name;
                ResNodePtr node;
            };
            std::vector< Child > children;

            String type;
            String name;
            ParamXMLPath paramPath;
            ParamXMLPath resultPath;
            int count;
            int cluster;
            int copyCount;
            bool external;

            ResNode();
        };
        typedef ResNode::ResNodePtr ResNodePtr;

        typedef ParamXMLTree::Node    XMLNode;
        typedef ParamXMLTree::NodePtr XMLNodePtr;
        typedef ParamXMLTree::NodeArray XMLNodeArray;
        typedef ParamXMLTree::ChildMap  XMLChildMap;
        typedef ParamXMLTree::AttributeMap XMLAttributeMap;

    protected:
        ResNodePtr m_rootNode;

        typedef std::map<String, ResNodePtr> ResNodeMap;
        ResNodeMap m_resNodeMap;
        std::vector<PhysicalResourceNode*> m_instanceList;

        ResourceFactory m_factory;

        // Constant value map
        typedef std::map<String, int> ConstantMap;
        ConstantMap m_constMap;

        // Get the path of 'Configuration' node specified by 'Simulator/@Configuration'
        ParamXMLPath GetConfigurationPath();
        
        // Load the constant section in XML
        void LoadConstantSection(const ParamXMLPath& path);

        // Load the parameter section
        void LoadParameterSection(const ParamXMLPath& path);

        // Load the structure section
        void LoadStructureSection(const ParamXMLPath& path);

        // Get attribute of 'Structure' node 
        int GetStructureNodeValue(const XMLNodePtr node, const String& attrName);
        String GetStructureNodeString(const XMLNodePtr node, const String& attrName);

        // Process structure nodes recursively
        void TraverseStructureNode(ResNodePtr resParent, XMLNodePtr xmlParent, int copyCount);

        // Construct each resource from the node map
        void ConstructResources();

        // Connect each resource
        void ConnectResources();

        // Initialize all nodes
        void InitializeNodes( PhysicalResourceNode::InitPhase phase );

        // Validate connection.
        void ValidateConnection();

        // --- Dump the loaded data for debug
        void Dump();

    public:
        ResourceBuilder();
        virtual ~ResourceBuilder();

        // Set external resource
        void SetExternalResource( ResNode* node );

        // Construct all physical resources
        void Construct();

        // Release all configuration data
        void Release();

        // Get all resources
        std::vector<PhysicalResourceNode*> GetAllResources()
        {
            return m_instanceList; 
        }

        // Get resources specified by the passed name.
        // Todo: add a version specified by the type name
        template <class T>
        bool GetResource(const String& name, PhysicalResourceArray<T>& resArray)
        {
            ResNodeMap::iterator ni = m_resNodeMap.find( name );
            if( ni == m_resNodeMap.end() )
                return false;

            ResNodePtr node = ni->second;
            size_t size = node->instances.size();
            resArray.Resize( size );
            for( size_t i = 0; i < size; i++ ){
                resArray[i] = dynamic_cast<T*>( node->instances[i] );
            }

            return true;
        }

        template <class T>
        bool GetResource(const String& name, T*& resPtr)
        {
            PhysicalResourceArray<T> resArray;
            if( !GetResource( name, resArray ) ){
                return false;
            }

            if( resArray.GetSize() != 1 ){
                return false;
            }

            resPtr = resArray[0];
            return true;
        }


        // Get resources specified by the passed type name.
        template <class T>
        bool GetResourceByType(const String& typeName, PhysicalResourceArray<T>& resArray)
        {
            resArray.Clear();
            for( size_t i = 0; i < m_instanceList.size(); ++i ){
                if( m_instanceList[i]->GetTypeName() == typeName ){
                    resArray.Add( dynamic_cast<T*>( m_instanceList[i] ) );
                }
            }

            return resArray.GetSize() != 0;
        }

        template <class T>
        bool GetResourceByType(const String& typeName, T*& resPtr)
        {
            PhysicalResourceArray<T> resArray;
            if( !GetResourceByType( typeName, resArray ) ){
                return false;
            }

            if( resArray.GetSize() != 1 ){
                return false;
            }

            resPtr = resArray[0];
            return true;
        }
    };

} // namespace Onikiri


#endif
