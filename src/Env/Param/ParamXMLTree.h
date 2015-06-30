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
// Parameter XML Tree
//

#ifndef ENV_PARAM_PARAM_XML_TREE_H
#define ENV_PARAM_PARAM_XML_TREE_H

#include "Env/Param/ParamXMLPath.h"

namespace Onikiri
{

    class ParamXMLTree
    {
    public:
        struct NodeStatus
        {
            bool stReadOnly;
            bool stArray;
            bool stRequireDefault;
            bool stDefault;
            bool stAttribute;
        };

        class StringHash
        {
        public:
            size_t operator()(const String& value) const;
        };

        struct Node;
        typedef boost::shared_ptr<Node>    NodePtr;
        typedef std::vector<NodePtr>       NodeArray;
        typedef unordered_map<String, NodeArray, StringHash> ChildMap;
        typedef unordered_map<String, NodePtr,  StringHash>  AttributeMap;
        typedef unordered_map<String, NodePtr,  StringHash>  ArrayPlaceMap;

        struct Node
        {
            Node();
            String        name;
            String        value;
            ChildMap      children;
            AttributeMap  attributes;
            ArrayPlaceMap arrayPlace;   // for PDB_Array palce holder
            NodeStatus    status;
            int           inputIndex;
            bool          accessed;
        };

        struct InputInfo
        {
            enum Type
            {
                IT_FILE,
                IT_STRING,
                IT_CMD
            };
            Type   type;
            String fileName;
        };

        ParamXMLTree();
        virtual ~ParamXMLTree();

        void LoadXMLFile( const String& fileName, bool isDefault );
        void LoadString( const String& str, bool isDefault, const String& signature );
        void LoadValue( const ParamXMLPath& path, const String& value );

        void ToXMLDoc(TiXmlDocument& doc);
        String ToXMLString();

        void Set( const ParamXMLPath& path, const String& value, bool forceOverWrite = false);
        bool Get( const ParamXMLPath& path,       String* value, NodeStatus* status = NULL );
        NodePtr GetNode( const ParamXMLPath& path, bool addNewNode = false );

        NodeArray* GetNodeArray( const ParamXMLPath& path, bool addNewNode = false );
        NodeArray* GetNodeArray( NodePtr parentNode, const String& childName, bool addNewNode = false );
        NodePtr    GetAttribute( NodePtr parentNode, const String& attributeName, bool addNewNode = false );

        NodePtr CreateNewNode( const String& name = "" );

        const std::vector<InputInfo>& GetInputList();
        bool GetSourceXMLFile( const ParamXMLPath& path, String& fileName );

    protected:
        NodePtr m_root;
        std::vector<InputInfo> m_inputList;

        void CheckXMLParseError( TiXmlDocument& doc, const String& from );
        void LoadXMLToTiXMLDoc( const String& fileName, TiXmlDocument& doc );

        void ConvertXMLToMap( NodePtr mapParent, TiXmlNode* xmlParent, int inputIndex, bool stDefault );
        void ConvertMapToXML( TiXmlNode* xmlParent, NodePtr mapParent );
        void CopyTree( NodePtr dst, NodePtr src );
    };
    

}   // namespace Onikiri

#endif  // #ifdef ENV_PARAM_PARAM_XML_TREE_H


