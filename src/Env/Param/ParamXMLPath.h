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
// Parameter XML Path
//

#ifndef ENV_PARAM_PARAM_XML_PATH_H
#define ENV_PARAM_PARAM_XML_PATH_H

namespace Onikiri
{

    class ParamXMLPath
    {
    public:

        enum NodeType
        {
            NT_NORMAL = 0,
            NT_ATTRIBUTE,
            NT_ARRAY,
            NT_COUNT_FUNCTION,
            NT_TEXT,
        };

        struct Node
        {
            Node();

            String   str;
            NodeType type;
            int      arrayIndex;
        };

        typedef boost::shared_ptr<Node> NodePtr;
        typedef std::list<NodePtr> Path;

        ParamXMLPath();
        ParamXMLPath( const ParamXMLPath& org );
        ParamXMLPath( const String& source );
        ParamXMLPath( const std::string& source );
        ParamXMLPath( const char* source );

        virtual ~ParamXMLPath();
        void Parse( Path& path, const String& str );
        String ToString() const;

        ParamXMLPath& operator +=( const ParamXMLPath& rhs );
        ParamXMLPath& operator +=( const String& rhs );
        ParamXMLPath& operator +=( const char* rhs );
        ParamXMLPath  operator +( const ParamXMLPath& rhs ) const;
        ParamXMLPath  operator +( const String& rhs ) const;
        ParamXMLPath  operator +( const char* rhs ) const;
        ParamXMLPath& operator =( const String& rhs );
        ParamXMLPath& operator =( const char* rhs );

        void Add( const String& path );
        void AddAttribute( const String& path );
        void AddArray( const String& path, int index );
        void SetArray( const ParamXMLPath& basePath, const String& relativePath, int index );

        NodePtr& Back();
        const NodePtr Back() const;
        void RemoveLast();

        // List compatible interfaces
        typedef Path::iterator iterator;
        typedef Path::const_iterator const_iterator;
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        void pop_back();
        NodePtr& back();
        const NodePtr& back() const;

    protected:
        Path m_path;
        typedef std::vector<String> TokenList;
        TokenList::const_iterator 
            NextToken(
                const TokenList& tokens, 
                TokenList::const_iterator cur, 
                const String& source
            );
    };
    

}   // namespace Onikiri

#endif  // #ifdef ENV_PARAM_PARAM_XML_PATH_H


