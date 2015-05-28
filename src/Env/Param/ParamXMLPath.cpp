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
#include "Env/Param/ParamXMLPath.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

ParamXMLPath::Node::Node()
{
    type = NT_NORMAL;
    arrayIndex = 0;
}

ParamXMLPath::ParamXMLPath()
{
}

ParamXMLPath::ParamXMLPath( const ParamXMLPath& org )
{
    m_path = org.m_path;
}

ParamXMLPath::ParamXMLPath( const String& source )
{
    Parse( m_path, source );
}

ParamXMLPath::ParamXMLPath( const std::string& source )
{
    Parse( m_path, source );
}

ParamXMLPath::ParamXMLPath( const char* source )
{
    Parse( m_path, source );
}

ParamXMLPath::~ParamXMLPath()
{
}


ParamXMLPath::TokenList::const_iterator 
    ParamXMLPath::NextToken(
        const TokenList& tokens,
        TokenList::const_iterator cur,
        const String& source
){
    ++cur;
    if( cur == tokens.end() ){
        THROW_RUNTIME_ERROR( "'%s' is an invalid path format.", source.c_str() );
    }
    return cur;
}

// Parse the 'source' as XPath format.
// Parsed results are added to the 'path'.
void ParamXMLPath::Parse( Path& path, const String& source )
{
    const vector<String>& tokens = source.split( "/", "[]@#()" );
    for( vector<String>::const_iterator i = tokens.begin();
         i != tokens.end();
         ++i
    ){
        NodePtr node( new Node );
        const String& token = *i;

        if( token == "@" ){

            // Attribute
            i = NextToken( tokens, i, source );
            node->type = NT_ATTRIBUTE;
            node->str = *i;

        }
        else if( token == "#" ){
            
            // Count function (abbreviation version)
            i = NextToken( tokens, i, source );
            node->type = NT_COUNT_FUNCTION;
            node->str = *i;

        }
        else{

            node->str = token;
            if( i+1 != tokens.end() ){

                const String& next = *NextToken( tokens, i, source );

                if( next == "[" ){
                    // Array
                    node->type = NT_ARRAY;

                    i = NextToken( tokens, i, source ); // i: '['
                    i = NextToken( tokens, i, source ); // i: '<number>'
                    node->arrayIndex = lexical_cast<int>(*i);

                    i = NextToken( tokens, i, source ); // i: ']'
                    if( *i != "]" ){
                        THROW_RUNTIME_ERROR( "'%s' has an invalid array format.", source.c_str() );
                    }
                }
                else if( next == "(" ){

                    // XPath functions or text()
                    const String& function = node->str;

                    if( function == "text" ){

                        // text() node.
                        node->type = NT_TEXT;
                        i = NextToken( tokens, i, source ); // i: '('
                        i = NextToken( tokens, i, source ); // i: ')'
                        if( *i != ")" ){
                            THROW_RUNTIME_ERROR( "'%s' has an invalid text format.", source.c_str() );
                        }

                    }
                    else if( function == "count" ){
                        
                        // Count function
                        node->type = NT_COUNT_FUNCTION;

                        i = NextToken( tokens, i, source ); // '('
                        i = NextToken( tokens, i, source ); // '<body>'
                        node->str = *i;
                        i = NextToken( tokens, i, source ); // ')'
                        if( *i != ")" ){
                            THROW_RUNTIME_ERROR( "'%s' has an invalid function format.", source.c_str() );
                        }

                    }
                    else{
                        THROW_RUNTIME_ERROR( "'%s' has an invalid path format.", source.c_str() );
                    }

                }
                else{
                    // Normal path
                }
            }   //  if( i+1 != tokens.end() ){
            else{
                // Normal path
            }
                
        }   // if( *i == "@" ){

        // Add a parsed node.
        path.push_back( node );

    }   // for( vector<String>::const_iterator i = tokens.begin();
}

String ParamXMLPath::ToString() const
{
    String ret;
    for( const_iterator i = begin(); i != end(); ++i ){
        ret += "/";

        const NodePtr& node = *i;
        switch( node->type ){
        case NT_ARRAY:
            ret += String().format( "%s[%d]", node->str.c_str(), node->arrayIndex );
            break;
        case NT_COUNT_FUNCTION:
            ret += String().format( "#%s", node->str.c_str() );
            break;
        case NT_ATTRIBUTE:
            ret += String().format( "@%s", node->str.c_str() );
            break;
        case NT_TEXT:
            ret += "text()";
            break;
        default:
            ret += node->str;
            break;
        }
    }
    return ret;
}

ParamXMLPath& ParamXMLPath::operator +=( const ParamXMLPath& rhs )
{
    for( const_iterator i = rhs.begin(); i != rhs.end(); ++i ){
        m_path.push_back( *i );
    }
    return *this;
}

ParamXMLPath& ParamXMLPath::operator +=( const String& rhs )
{
    Parse( m_path, rhs );
    return *this;
}

ParamXMLPath& ParamXMLPath::operator +=( const char* rhs )
{
    Parse( m_path, rhs );
    return *this;
}

ParamXMLPath ParamXMLPath::operator +( const ParamXMLPath& rhs ) const
{
    ParamXMLPath tmp = *this;
    tmp += rhs;
    return tmp;
}

ParamXMLPath ParamXMLPath::operator +( const String& rhs ) const
{
    ParamXMLPath tmp = *this;
    tmp += rhs;
    return tmp;
}

ParamXMLPath ParamXMLPath::operator +( const char* rhs ) const
{
    ParamXMLPath tmp = *this;
    tmp += rhs;
    return tmp;
}

ParamXMLPath& ParamXMLPath::operator = ( const String& rhs )
{
    m_path.clear();
    Parse( m_path, rhs );
    return *this;
}

ParamXMLPath& ParamXMLPath::operator = ( const char* rhs )
{
    m_path.clear();
    Parse( m_path, rhs );
    return *this;
}

ParamXMLPath::NodePtr& ParamXMLPath::Back()
{
    return back();
}

const ParamXMLPath::NodePtr ParamXMLPath::Back() const
{
    return back();
}

void ParamXMLPath::RemoveLast()
{
    pop_back();
}

void ParamXMLPath::Add( const String& path )
{
    Parse( m_path, path );
}

void ParamXMLPath::AddAttribute( const String& path )
{
    NodePtr node( new Node );
    node->type = NT_ATTRIBUTE;
    node->str = path;
    m_path.push_back( node );
}

void ParamXMLPath::AddArray( const String& path, int index )
{
    Parse( m_path, path );
    m_path.back()->type = NT_ARRAY;
    m_path.back()->arrayIndex = index;
}

void ParamXMLPath::SetArray( const ParamXMLPath& basePath, const String& relativePath, int index )
{
    *this = basePath;
    AddArray( relativePath, index );
}


//
// -- list compatible 
//
void ParamXMLPath::pop_back()
{
    if( m_path.size() > 0 )
        m_path.pop_back();
}

ParamXMLPath::NodePtr& 
ParamXMLPath::back()
{
    return m_path.back();
}

const ParamXMLPath::NodePtr& 
ParamXMLPath::back() const
{
    return m_path.back();
}

ParamXMLPath::iterator ParamXMLPath::begin()
{
    return m_path.begin();
}

ParamXMLPath::iterator ParamXMLPath::end()
{
    return m_path.end();
}

ParamXMLPath::const_iterator ParamXMLPath::begin() const
{
    return m_path.begin();
}

ParamXMLPath::const_iterator ParamXMLPath::end() const
{
    return m_path.end();
}
