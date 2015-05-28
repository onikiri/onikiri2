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
#include "Env/Param/ParamXMLTree.h"
#include "Env/Param/ParamXMLPrinter.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

// A hash function for 'String'
size_t ParamXMLTree::StringHash::operator()(const String& value) const
{
    if( value.size() < 3 ){
        return 0;
    }
    else{
        return 
            ((value.at(0) << 8) | value.at(1)) ^
            value.at(2);
    }
}

ParamXMLTree::Node::Node()
{
    status.stArray          = false;
    status.stReadOnly       = true;
    status.stRequireDefault = true;
    status.stDefault        = true;
    status.stAttribute      = false;
    inputIndex = -1;
    accessed = false;
}

ParamXMLTree::ParamXMLTree()
{
    m_root = NodePtr( new Node() );

    // Default status
    m_root->name = "root";
}

ParamXMLTree::~ParamXMLTree()
{
}

ParamXMLTree::NodePtr ParamXMLTree::CreateNewNode( const String& name )
{
    Node* node = new Node();
    node->name = name;
    return NodePtr( node );
}

// Check parse error.
void ParamXMLTree::CheckXMLParseError( TiXmlDocument& doc, const String& from )
{
    if( doc.Error() ){
        String err;
        err.format(
            "Could not load from '%s'\n%s\nline:%d col:%d\n",
            from.c_str(),
            doc.ErrorDesc(),
            doc.ErrorRow(),
            doc.ErrorCol()
        );
        THROW_RUNTIME_ERROR( err.c_str() );
    }
}

// Copy node contents (attributes/nodes) from the 'src' to the 'dst'.
void ParamXMLTree::CopyTree( NodePtr dst, NodePtr src )
{
    dst->inputIndex = src->inputIndex;
    dst->status = src->status;
    dst->value = src->value;
    dst->name = src->name;

    // Copy attributes
    for( AttributeMap::iterator i = src->attributes.begin();
        i != src->attributes.end();
        ++i
    ){
        NodePtr newNode( new Node );
        *newNode = *i->second;
        dst->attributes[ i->first ] = newNode;
    }

    // Copy children
    for( ChildMap::iterator i = src->children.begin();
        i != src->children.end();
        ++i
    ){
        const String& name = i->first;
        NodeArray& children = i->second;
        for( size_t index = 0; index < children.size(); index++ ){
            NodePtr newNode( new Node );
            dst->children[ name ].push_back(newNode);
            CopyTree( newNode, children[ index ] );
        }
    }

    // Copy array placeholder
    for( ArrayPlaceMap::iterator i = src->arrayPlace.begin();
        i != src->arrayPlace.end();
        ++i
    ){
        NodePtr newNode( new Node );
        CopyTree( newNode, i->second );
        dst->arrayPlace[ i->first ] = newNode;
    }
}


// Convert each Tiny XML node to an internal map node recursively.
void ParamXMLTree::ConvertXMLToMap( 
    NodePtr mapParent, TiXmlNode* xmlParent, int inputIndex, bool stDefault )
{
    map<String, size_t> nodeCount;
    
    for( TiXmlElement* childElement = xmlParent->FirstChildElement();
         childElement != NULL;
         childElement = childElement->NextSiblingElement()
     ){
        
        NodePtr childNode( new Node() );

        // The status of a parent node is propagated to a child.
        const char* childName = childElement->Value();
        childNode->name = childName;
        childNode->status = mapParent->status;
        childNode->status.stDefault = stDefault;
        childNode->inputIndex = inputIndex;
        
        // Is a child node is a placeholder of an array?
        const char* pdbArrayStr = childElement->Attribute( "PDB_Array" );
        bool arrayPlace = pdbArrayStr ? lexical_cast<bool>(pdbArrayStr) : false;

        // Is a child node is a part of an array?
        bool arrayNode = 
            mapParent->arrayPlace.find( childName ) != mapParent->arrayPlace.end();

        // Convert an element
        if( arrayPlace ){
            // A node is a placeholder of 'array' (@PDB_Array='1'), 
            mapParent->arrayPlace[ childName ] = childNode;
        }
        else if( arrayNode ){
            // A node is 'array' (@PDB_Array='1'), 
            CopyTree( childNode, mapParent->arrayPlace[ childName ] );
            mapParent->children[ childName ].push_back( childNode );
            childNode->status.stArray = true;
        }
        else{
            // If a node is not 'array' (@PDB_Array='0'), already loaded nodes are over written.
            size_t index = nodeCount[ childName ];
            NodeArray& list = mapParent->children[ childName ];
            if( index >= list.size() ){
                // Add a new node
                list.resize( index + 1 );
                list[index] = childNode;
            }
            else{
                // An already loaded node is over written.
                NodeStatus newStatus = childNode->status;

                if(list[index]->status.stDefault){
                    // You cannot overwrite status 'stDefault'.
                    // If an old node is a 'default' node, 
                    // a new node is also 'default' node.
                    newStatus.stDefault = true;
                }

                childNode = list[index];
                childNode->status = newStatus;
            }

            nodeCount[ childName ]++;
        }
        
        // Convert attributes
        for( TiXmlAttribute* attr = childElement->FirstAttribute();
             attr != NULL;
             attr = attr->Next() 
        ){
            const String& name  = attr->Name();
            const String& value = attr->Value();
            NodePtr attribute( new Node() );
            attribute->name  = name;
            attribute->value = value;
            attribute->inputIndex = inputIndex;
            childNode->attributes[ name ] = attribute;

            // Special internal attributes
            if( name == "PDB_Array" ){
                arrayPlace = lexical_cast<bool>(value);
                childNode->status.stArray = arrayPlace;
                attribute->accessed = true;
            }
            else if( name == "PDB_ReadOnly" ){
                childNode->status.stReadOnly = lexical_cast<bool>(value);
                attribute->accessed = true;
            }
            else if( name == "PDB_RequireDefaultParam" ){
                childNode->status.stRequireDefault = lexical_cast<bool>(value);
                attribute->accessed = true;
            }
        }

        // Convert child nodes
        ConvertXMLToMap( childNode, childElement, inputIndex, stDefault );
    }
}

// Convert each internal map node to a tiny XML node recursively.
void ParamXMLTree::ConvertMapToXML( TiXmlNode* xmlParent, NodePtr mapParent )
{

    for( ChildMap::iterator children = mapParent->children.begin();
         children != mapParent->children.end();
         ++children
     ){
         
         // Get siblings, which are elements with same names.
         NodeArray& siblingList = children->second;
         for( size_t index = 0; index < siblingList.size(); index++ ){

            NodePtr sibling = siblingList[ index ];
            
            TiXmlElement* node = new TiXmlElement( sibling->name );
            xmlParent->LinkEndChild( node );
             
            // Set text data.
            if( sibling->value != "" && sibling->children.size() == 0 ){

                // Escape "<" to avoid corruption of XML format.
                String value = sibling->value;
                value.find_and_replace( "<", "&lt;" );
                
                TiXmlText* text = new TiXmlText( value );
                node->LinkEndChild( text );
            }

            // Set attributes
            AttributeMap &attr = sibling->attributes;
            for( AttributeMap::iterator i = attr.begin();
                 i != attr.end();
                 ++i
             ){
                 node->SetAttribute( i->first, i->second->value );
            }

            ConvertMapToXML( node, sibling );
         }
    }


}

// Load a passed XMLfile.
void ParamXMLTree::LoadXMLFile( const String& fileName, bool stDefault )
{
    TiXmlDocument doc;
    doc.LoadFile( fileName );
    CheckXMLParseError( doc, fileName );

    int inputIndex = (int)m_inputList.size();
    ConvertXMLToMap( m_root, &doc, inputIndex, stDefault );
    InputInfo info;
    info.fileName = fileName;
    info.type = InputInfo::IT_FILE;
    m_inputList.push_back( info);
}

// Load a passed raw XML string.
void ParamXMLTree::LoadString( const String& xmlString, bool stDefault, const String& signature )
{
    TiXmlDocument doc;
    doc.Parse( xmlString );
    CheckXMLParseError( doc, signature );

    int inputIndex = (int)m_inputList.size();
    InputInfo info;
    info.fileName = signature;
    info.type = InputInfo::IT_STRING;
    m_inputList.push_back( info);
    ConvertXMLToMap( m_root, &doc, inputIndex, stDefault );
}

// Load a passed value.
void ParamXMLTree::LoadValue( const ParamXMLPath& path, const String& value )
{
    int inputIndex = (int)m_inputList.size();
    InputInfo info;
    info.type = InputInfo::IT_CMD;
    m_inputList.push_back( info);

    Set( path, value, true );
    GetNode( path )->inputIndex = inputIndex;
}

// Convert the map to a Tiny XML document object.
void ParamXMLTree::ToXMLDoc( TiXmlDocument& doc )
{
    ConvertMapToXML( &doc, m_root );
}

// Convert the map to a string.
String ParamXMLTree::ToXMLString()
{
    TiXmlDocument dstDoc;

    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );  
    dstDoc.LinkEndChild( decl ); 

    ConvertMapToXML( &dstDoc, m_root );

    // Use an original TiXmlPrinter printer;
    ParamXMLPrinter printer;
    dstDoc.Accept( &printer );
    return String( printer.CStr() );
}

//
void ParamXMLTree::Set( const ParamXMLPath& path, const String& value, bool forceOverWrite )
{
    NodePtr node = m_root;
    for( ParamXMLPath::const_iterator i = path.begin();
         i != path.end();
         ++i
    ){
        ParamXMLPath::NodePtr pathNode = *i;
        if( pathNode->type == ParamXMLPath::NT_ARRAY ){
            // Array
            ChildMap::iterator next = node->children.find( pathNode->str );
            if( next == node->children.end() ){
                node->children[ pathNode->str ] = NodeArray();
                next = node->children.find( pathNode->str );
            }

            NodeArray& nodeList = next->second;
            size_t arrayIndex  = pathNode->arrayIndex;
            for( size_t i = nodeList.size(); i < arrayIndex + 1; i++ ){
                NodePtr newNode = NodePtr( new Node() );
                newNode->status = node->status;
                newNode->name   = pathNode->str;
                nodeList.push_back( newNode );
            }
            node = nodeList[ arrayIndex ];

        }
        else if( pathNode->type == ParamXMLPath::NT_ATTRIBUTE ){
            // attribute
            NodePtr newAttr( new Node() );
            newAttr->name  = pathNode->str;
            newAttr->value = value;
            newAttr->accessed = true;

            AttributeMap::iterator oldAttr = 
                node->attributes.find( pathNode->str );

            if( oldAttr == node->attributes.end() ){
                node->attributes[ pathNode->str ] = newAttr;
            }
            else{
                if( !forceOverWrite &&
                    node->status.stReadOnly && 
                    oldAttr->second->value != value
                ){
                    THROW_RUNTIME_ERROR( 
                        "The path '%s' is read only. The value is changed from '%s' to '%s'",
                        path.ToString().c_str(),
                        oldAttr->second->value.c_str(),
                        value.c_str()
                    );
                }

                oldAttr->second = newAttr;
            }
            return;
        }
        else if( pathNode->type == ParamXMLPath::NT_COUNT_FUNCTION ){
            // Count function
            // ???
            RUNTIME_WARNING( "Count function?" );
            return;
        }
        else if( pathNode->type == ParamXMLPath::NT_TEXT ){
            
            // Text
            node->accessed = true;

            // When 'text()' is referenced, its node must be already created,
            // thus a new node does not need to be created.
            // Text data is written to 'value' of its node.
            if( !forceOverWrite &&
                node->status.stReadOnly && 
                node->value != value
            ){
                THROW_RUNTIME_ERROR( 
                    "The path '%s' is read only. The value is changed from '%s' to '%s'",
                    path.ToString().c_str(),
                    node->value.c_str(),
                    value.c_str()
                );
            }
            node->value = value;

            return;
        }
        else{
            // Normal node
            ChildMap::iterator next = node->children.find( pathNode->str );
            if( next == node->children.end() ){
                node->children[ pathNode->str ] = NodeArray();
                next = node->children.find( pathNode->str );
            }

            NodeArray& nodeList = next->second;
            if( nodeList.size() < 1 ){
                NodePtr newNode = NodePtr( new Node() );
                newNode->status = node->status;
                newNode->name   = pathNode->str;
                nodeList.push_back( newNode );
            }
            node = nodeList[0];
        }
    }
}

//
bool ParamXMLTree::Get( const ParamXMLPath& path, String* value, NodeStatus* status )
{
    NodePtr node = m_root;
    for( ParamXMLPath::const_iterator i = path.begin();
         i != path.end();
         ++i
    ){
        ParamXMLPath::NodePtr pathNode = *i;
        if( pathNode->type == ParamXMLPath::NT_ARRAY ){
            // Array
            ChildMap::iterator next = node->children.find( pathNode->str );
            if( next != node->children.end() ){
                if( (size_t)pathNode->arrayIndex >= next->second.size()  ){
                if(status)
                    *status = node->status;
                    return false;
                }
                node = next->second[pathNode->arrayIndex];
            }
            else{
                if(status)
                    *status = node->status;
                return false;
            }
        }
        else if( pathNode->type == ParamXMLPath::NT_ATTRIBUTE ){
            // attribute
            AttributeMap::iterator attribute =
                node->attributes.find( pathNode->str );

            if( attribute != node->attributes.end() ){
                if(status)
                    *status = attribute->second->status;
                *value = attribute->second->value;
                attribute->second->accessed = true;
                return true;
            }
            else{
                if(status)
                    *status = node->status;
                return false;
            }
        }
        else if( pathNode->type == ParamXMLPath::NT_COUNT_FUNCTION ){
            // Count function
            if(status)
                *status = node->status;
            ChildMap::iterator next = node->children.find( pathNode->str );
            if( next != node->children.end() ){
                *value = lexical_cast<String>( next->second.size() );
                return true;
            }
            else{
                return false;
            }
        }
        else if( pathNode->type == ParamXMLPath::NT_TEXT ){
            // Text
            if(status){
                *status = node->status;
            }
            *value = node->value;
            return true;
        }
        else{
            // Normal node
            ChildMap::iterator next = node->children.find( pathNode->str );
            if( next != node->children.end() ){
                if( next->second.size() == 0 ){
                    THROW_RUNTIME_ERROR( 
                        "The node map is borken.\n"
                        "'%s' has not child.\n"
                        "path: %s",
                        pathNode->str.c_str(),
                        path.ToString().c_str()
                    );
                }
                else if( next->second.size() != 1 ){
                    THROW_RUNTIME_ERROR( 
                        "The specified path '%s' is array.\n"
                        "path: %s",
                        pathNode->str.c_str(),
                        path.ToString().c_str()
                    );
                }

                node = next->second[0];
            }
            else{
                if(status)
                    *status = node->status;
                return false;
            }
        }
    }
    if(status)
        *status = node->status;
    return false;
}


ParamXMLTree::NodePtr 
ParamXMLTree::GetNode( const ParamXMLPath& path, bool addNewNode )
{
    NodePtr node = m_root;
    for( ParamXMLPath::const_iterator i = path.begin();
         i != path.end();
         ++i
    ){
        ParamXMLPath::NodePtr pathNode = *i;
        if( pathNode->type == ParamXMLPath::NT_ARRAY ){
            // Array
            ChildMap::iterator child = node->children.find( pathNode->str );

            if( child == node->children.end() ){
                if( addNewNode ){
                    // Can not find a child node and add a new node
                    node->children[ pathNode->str ] = NodeArray();
                    child = node->children.find( pathNode->str );
                }
                else{
                    // Failed
                    return NodePtr();
                }
            }

            NodeArray& childList = child->second;
            size_t arrayIndex  = pathNode->arrayIndex;

            if( childList.size() <= arrayIndex ){
                if( addNewNode ){
                    //
                    for( size_t i = childList.size(); i < arrayIndex + 1; i++ ){
                        NodePtr newNode = NodePtr( new Node() );
                        newNode->status = node->status;
                        newNode->name   = pathNode->str;
                        childList.push_back( newNode );
                    }
                }
                else{
                    // Failed
                    return NodePtr();
                }
            }

            node = childList[ arrayIndex ];

        }
        else if( pathNode->type == ParamXMLPath::NT_ATTRIBUTE ){
            // attribute
            AttributeMap::iterator attribute =
                node->attributes.find( pathNode->str );

            if( attribute != node->attributes.end() ){
                return attribute->second;
            }
            else{
                if( addNewNode ){
                    if( node->status.stReadOnly ){
                        THROW_RUNTIME_ERROR( 
                            "The path '%s' is read only.",
                            path.ToString().c_str()
                        );
                    }

                    NodePtr newAttribute( new Node() );
                    newAttribute->name  = pathNode->str;
                    node->attributes[ pathNode->str ] = newAttribute;
                    return newAttribute;
                }
                else{
                    return NodePtr();
                }
            }
        }
        else if( pathNode->type == ParamXMLPath::NT_COUNT_FUNCTION ){
            // Count function
            THROW_RUNTIME_ERROR( "Count function is not support." );
            return NodePtr();
        }
        else{
            // Normal node
            ChildMap::iterator child = node->children.find( pathNode->str );
            if( child == node->children.end() ){
                if( addNewNode ){
                    node->children[ pathNode->str ] = NodeArray();
                    child = node->children.find( pathNode->str );
                }
                else{
                    return NodePtr();
                }
            }

            NodeArray& nodeList = child->second;
            if( nodeList.size() < 1 ){
                if( addNewNode ){
                    NodePtr newNode = NodePtr( new Node() );
                    newNode->status = node->status;
                    newNode->name   = pathNode->str;
                    nodeList.push_back( newNode );
                }
                else{
                    return NodePtr();
                }
            }
            node = nodeList[0];
        }
    }
    return node;
}

ParamXMLTree::NodeArray* 
ParamXMLTree::GetNodeArray( NodePtr parentNode, const String& childName, bool addNewNode )
{
    ChildMap::iterator child = 
        parentNode->children.find( childName );
    if( child != parentNode->children.end() )
        return &child->second;

    if( addNewNode )
        return &parentNode->children[ childName ];
    else
        return NULL;
}

ParamXMLTree::NodeArray* 
ParamXMLTree::GetNodeArray( const ParamXMLPath& orgPath, bool addNewNode )
{
    ParamXMLPath path = orgPath;
    path.pop_back();

    NodePtr node = GetNode( path, addNewNode );
    if( node == NULL ){
        return NULL;
    }

    return GetNodeArray( node, orgPath.back()->str, addNewNode );
}

ParamXMLTree::NodePtr
ParamXMLTree::GetAttribute( NodePtr parentNode, const String& attributeName, bool addNewNode )
{
    AttributeMap::iterator attribute = 
        parentNode->attributes.find( attributeName );
    if( attribute != parentNode->attributes.end() )
        return attribute->second;

    if( addNewNode )
        return parentNode->attributes[ attributeName ];
    else
        return NodePtr();
}



const std::vector<ParamXMLTree::InputInfo>& ParamXMLTree::GetInputList()
{
    return m_inputList;
}

bool ParamXMLTree::GetSourceXMLFile( const ParamXMLPath& path, String& fileName )
{
    NodePtr node = GetNode( path );
    if( node == NULL ){
        return false;
    }

    if( node->inputIndex == -1 || node->inputIndex >= (int)m_inputList.size() ){
        return false;
    }

    InputInfo& info = m_inputList[node->inputIndex];
    if( info.type != InputInfo::IT_FILE )
        return false;

    fileName = info.fileName;
    return true;
}
