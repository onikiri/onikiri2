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
// Parameter data base
//

#include <pch.h>
#include "Env/Param/ParamDB.h"
#include "DefaultParam.h"
#include "Env/Param/ParamXMLPathReductionMap.h"

using namespace std;
using namespace boost;

// Global instance
namespace Onikiri
{
    ParamDB g_paramDB;
}


using namespace Onikiri;

ParamDB::ParamDB()
{
    m_initialized = false;
    m_userParamPassed = false;
}

ParamDB::~ParamDB()
{
    
}

bool ParamDB::Initialize( const String& initialPath )
{
    if(m_initialized){
        THROW_RUNTIME_ERROR( "Initialize() is called more than once." );
    }

    m_initialPath = initialPath;
    m_tree.LoadString( g_defaultParam, true, "DefaultParam" );
    m_initialized = true;
    return true;
}

void ParamDB::Finalize()
{
}

const vector<ParamXMLTree::InputInfo>&
    ParamDB::GetXMLFileNames()
{
    return m_tree.GetInputList();
}

String ParamDB::CompletePath( const String& target, const String& basePath )
{
    using namespace filesystem;
    return
        absolute( 
            path( (const string&)target   ), 
            path( (const string&)basePath )
        ).string();
}

// Remove the file name part from the path string
String ParamDB::RemoveFileName(const String& path)
{
    String ret;
    int indexBs = (int)path.rfind( "\\" );
    int indexSl = (int)path.rfind( "/" );
    int indexPath = indexSl > indexBs ? indexSl : indexBs;
    if(indexPath == -1)
        ret = "./";
    else
        ret.assign(path.begin(), path.begin() + indexPath + 1);
    return ret;
}

// Load one XML file.
void ParamDB::LoadXMLFile(const String& fileName)
{
    const String& fileFullPathName = 
        CompletePath( fileName, m_initialPath );

    // Import temporary tree
    ParamXMLTree curTree;
    curTree.LoadXMLFile( fileFullPathName, false );

    // The fileBasePath is always full path.
    String fileBasePath = RemoveFileName( fileFullPathName );
    ParamXMLPath xmlBasePath( "/Session/Import/" );

    vector<ParamXMLPath> importedPathList;

    for(int i = 0; ;i++){
        ParamXMLPath path = xmlBasePath;
        path.AddArray( "File", i );
        path.AddAttribute( "Path" );
        
        String importFileName;
        bool hit = curTree.Get( path, &importFileName );
        if( !hit )
            break;

        importedPathList.push_back( path );

        const String& importFullPath = 
            CompletePath( importFileName, fileBasePath );
        LoadXMLFile( importFullPath );
    }

    AddExternalParameterToList( "FileName", fileName );

    // Body
    // The file path passed to the m_tree must be full path, 
    // because the return value of the m_tree.GetSourceXMLFile() 
    // is made from this file path.
    m_tree.LoadXMLFile( fileFullPathName, false );
    m_isLoaded = true;

    // Touch an attribute for an access footprint
    // (XMLNode::accessed
    for( size_t i = 0; i < importedPathList.size(); i++ ){
        String tmpString;
        m_tree.Get( importedPathList[i], &tmpString );
    }

    m_userParamPassed = true;
}

// Load parameters specified by the command line arguments
void ParamDB::LoadParameters(const vector<String>& argList)
{
    for( vector<String>::const_iterator i = argList.begin() + 1;
         i != argList.end(); 
         ++i 
    ){
        const String& arg = *i;

        if( arg.rfind(".xml") == arg.size() - 4 ){
            LoadXMLFile( arg );
        }
        else if( arg.find("-x") == 0 ){
            ++i;
            if(i == argList.end())
                THROW_RUNTIME_ERROR("'-x' option requires parameter expression.");
            AddParameter( *i );
        }
    }
    m_userParamPassed = true;
}

// Add a parameter (XML file or command line parameter) to the input list.
void ParamDB::AddExternalParameterToList(const String& name, const String& value)
{
    ParamXMLTree::NodeArray* inputList = 
        m_tree.GetNodeArray( "/Session/InputList/Input", true );

    ParamXMLTree::NodePtr node = m_tree.CreateNewNode( "Input" );
    inputList->push_back( node );

    ParamXMLTree::NodePtr attr = m_tree.CreateNewNode( name );
    attr->value = value;
    attr->accessed = true;
    node->attributes[name] = attr;
}

// Add a command line parameter to the XML tree.
void ParamDB::AddParameter(const String& paramExp)
{
    const vector<String>& exp = paramExp.split("=");
    if(exp.size() != 2 || exp[0].size() == 0){
        THROW_RUNTIME_ERROR( 
            "'%s' is invalid expressoion.", 
            paramExp.c_str() );
    }

    // Expand the XML path
    String strPath;
    if(exp[0].at(0) == '/')
        strPath = exp[0];
    else
        strPath = String("/Session/") + exp[0];
    
    for( int i = 0; i < g_pathReductionMapSize; i++ ){
        String reductionPath = g_pathReductionMap[i].reductionPath;
        size_t pos =  strPath.find( reductionPath );
        if( pos == 0 ){
            strPath.replace( 
                pos, reductionPath.length() , 
                g_pathReductionMap[i].fullPath
            );
        }
    }

    ParamXMLPath path = strPath;
    
    String value;
    if( !m_tree.Get( path, &value ) ){
        THROW_RUNTIME_ERROR(
            "Parameter passed by command line argument is not valid.\n"
            "'%s' is not found in default parameter.\n",
            path.ToString().c_str() 
        );
    }

    m_tree.LoadValue( path, exp[1] );

    AddExternalParameterToList( "Expression", paramExp );
    m_userParamPassed = true;
}

// Add a user default parameter (XML string) to the tree.
void ParamDB::AddUserDefaultParam(const String& xmlString)
{
    if( m_userParamPassed ){
        THROW_RUNTIME_ERROR(
            "Added the user defined default parameter after loading user parameters."
            "All user defined default parameter must be added before loading user parameters."
        );
    }

    m_tree.LoadString( xmlString, true, "User" );
}

// Test whether the 'str' match one of the 'filterList'
bool ParamDB::MatchFilterList( const String& str, const std::vector<String>& filterList ) const
{
    for( size_t i = 0; i < filterList.size(); i++ ){
        if( str.find( filterList[i] ) != String::npos )
            return true;
    }

    return false;
}

void ParamDB::CheckResultXML( const String& path, XMLNodePtr node, int* warningCount )
{
    static const int MAX_WARNING_NUM = 8;
    if( (*warningCount) > MAX_WARNING_NUM ){
        return;
    }

    for( XMLChildMap::iterator children = node->children.begin();
         children != node->children.end();
         ++children
     ){
         
         // Get siblings, which are elements with same name.
         XMLNodeArray& siblings = children->second;
         for( size_t index = 0; index < siblings.size(); index++ ){

            XMLNodePtr sibling = siblings[ index ];

            // Set the attributes
            XMLAttributeMap &attr = sibling->attributes;
            for( XMLAttributeMap::iterator i = attr.begin();
                 i != attr.end();
                 ++i
             ){
                 if( !i->second->accessed && !IsInException() ){
                    (*warningCount)++;
                    RUNTIME_WARNING(
                         "The attribute '%s' have never been accessed. "
                         "The attribute name may be wrong.", 
                         (path + sibling->name + "/@"+ i->second->name).c_str()
                    );

                    if( (*warningCount) > MAX_WARNING_NUM ){
                        RUNTIME_WARNING( "Too many warnings. Omit subsequent warnings...");

                        return;
                    }
                 }
            }

            CheckResultXML( path + sibling->name + "/", sibling, warningCount );
         }

    }
}

// Filter result XML tree recursively
// return whether 'node' has valid children or not
bool ParamDB::FilterResultXML( const String& path, XMLNodePtr node, const vector<String>& filterList )
{
    if( MatchFilterList( path + node->name, filterList ) ){
        return true;
    }

    bool validNode = false;
    for( XMLChildMap::iterator children = node->children.begin();
         children != node->children.end();
     ){
         
         // Get siblings, which are elements with same name.
         XMLNodeArray& siblings = children->second;
         bool validChild = false;
         for( size_t index = 0; index < siblings.size(); index++ ){

            XMLNodePtr sibling = siblings[ index ];
            if( MatchFilterList( path + sibling->name, filterList ) ){
                validChild = true;
                continue;
            }
            
            // Set the attributes
            XMLAttributeMap &attr = sibling->attributes;
            for( XMLAttributeMap::iterator i = attr.begin();
                 i != attr.end();
             ){
                 if( MatchFilterList( path + i->second->name, filterList ) ){
                     ++i;
                 }
                 else{
                     i = attr.erase( i );
                 }
            }

            bool validChildren = FilterResultXML( path + sibling->name + "/", sibling, filterList );
            if( validChildren || attr.size() > 0 ){
                validChild = true;
            }
         }

         if( validChild ){
             validNode = true;
             ++children;
         }
         else{
            // printf("%s\n", children->second[0]->name.c_str() );
             children = node->children.erase( children );
         }
    }

    return validNode;
}

// Dump XML
String ParamDB::DumpResultXML( const String& level, const String& filter  )
{
    XMLNodePtr session = m_tree.GetNode( "/Session" );

    // Remove configuration nodes other than the current configuration.
    String currentConfigName = 
        m_tree.GetNode( "/Session/Simulator/@Configuration" )->value;
    XMLNodePtr configNode = 
        m_tree.GetNode( "/Session/Simulator/Configurations" );
    for( XMLChildMap::iterator child = configNode->children.begin();
         child != configNode->children.end();
    ){
        if( child->first == currentConfigName ){
            ++child;
        }
        else{
            child = configNode->children.erase( child );
        }
    }
    
    // Check untouched nodes.
    int warningCount = 0;
    CheckResultXML( "/Session/", session, &warningCount );

    // Apply the filter to the result XML.
    vector<String> filterList = filter.split( "," );
    if( filterList.size() > 0 ){
        FilterResultXML( "/Session/", session, filterList );
    }

    if( level == "Minimum" ){
        XMLNodePtr session = m_tree.GetNode( "/Session" );
        session->children.erase( "Simulator" );
        session->children.erase( "Emulator" );
        session->children.erase( "Environment" );
        session->children.erase( "Import" );

        ParamXMLTree::NodePtr result = m_tree.GetNode( "/Session/Result" );
        if( result ){
            result->children.erase( "Resource" );
        }

        return m_tree.ToXMLString();
    }
    else if( level == "BasicResult" ){
        ParamXMLTree::NodePtr session = m_tree.GetNode( "/Session" );
        session->children.erase( "Simulator" );
        session->children.erase( "Emulator" );
        session->children.erase( "Environment" );
        session->children.erase( "Import" );

        ParamXMLTree::NodePtr res = m_tree.GetNode( "/Session/Result/Resource" );
        if( res ){
            res->children.erase( "PipelinedExecUnit" );
            res->children.erase( "ExecUnit" );
            res->children.erase( "MemExecUnit" );
            res->children.erase( "Core" );
            res->children.erase( "CheckpointMaster" );
            res->children.erase( "ExecLatencyInfo" );
            res->children.erase( "Thread" );
            res->children.erase( "PHT" );
            res->children.erase( "Scheduler" );
            res->children.erase( "GlobalHistory" );
            res->children.erase( "GlobalClock" );
            res->children.erase( "RAS" );
        }
    
        return m_tree.ToXMLString();
    }
    else if( level == "Result" ){
        ParamXMLTree::NodePtr session = m_tree.GetNode( "/Session" );
        session->children.erase( "Simulator" );
        session->children.erase( "Emulator" );
        session->children.erase( "Environment" );
        session->children.erase( "Import" );
        return m_tree.ToXMLString();
    }
    else if( level == "Detail"){
        return m_tree.ToXMLString();
    }
    else{
        THROW_RUNTIME_ERROR(
            "An unknown ouput level '%s' is specified in the"
            "'/Session/Environment/OutputXML/@Level'\n"
            "This parameter must be one of the following strings : \n"
            "[Detail, BasicResult, Result, Minimum]",
            level.c_str()
        );
        return m_tree.ToXMLString();
    }
}

//
bool ParamDB::GetSourceXMLFile( const ParamXMLPath& parameterPath, String& sourceFile )
{
    if(m_isLoaded){
        return m_tree.GetSourceXMLFile( parameterPath, sourceFile );
    }

    return false;
}

void ParamDB::Set( const ParamXMLPath& path, const String& val )
{
    m_tree.Set( path, val );
}

bool ParamDB::Get(const ParamXMLPath& path, String* value, bool required)
{
    ParamXMLTree::NodeStatus status;
    bool ret = m_tree.Get( path, value, &status );

    if( !ret && required && status.stRequireDefault ){
        THROW_RUNTIME_ERROR(
            "error: %s is not found in the default parameters.",
            path.ToString().c_str() 
        );
    }

    return ret;
}

size_t ParamDB::GetElementCount(const ParamXMLPath& path)
{
    ParamXMLTree::NodeArray* nodeArray =
        m_tree.GetNodeArray( path );
    if( !nodeArray )
        return 0;

    return nodeArray->size();
}

// Get the XML tree
ParamXMLTree& ParamDB::GetXMLTree()
{
    return m_tree;
}



//
// --- Special versions
//

void ParamDB::ToParam( bool* dst, const ParamXMLPath& path, const String& str )
{
    ToParamRaw( dst, path, str );
}

void ParamDB::ToParam( String* dst, const ParamXMLPath& path, const String& str )
{
    ToParamRaw( dst, path, str );
}

void ParamDB::ToParam( std::string* dst, const ParamXMLPath& path, const String& str )
{
    ToParamRaw( dst, path, str );
}
