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
// Custom XML printer for tiny XML
//
// Modified the original 'TiXmlPrinter' to insert a line break for each attribute.
// # Do not inherit the original 'TiXmlPrinter'Å@because all member
//   values are private and cannot use those.
//

#ifndef ENV_PARAM_PARAM_XML_PRINTER_H
#define ENV_PARAM_PARAM_XML_PRINTER_H

namespace Onikiri
{
    class ParamXMLPrinter : public TiXmlVisitor
    {

    public:

        ParamXMLPrinter()
        {
            m_simpleTextPrint = false;
            m_indetStr = "  ";
            m_depth = 0;
        }

        const char* CStr() const
        {
            return m_buffer.c_str();
        }

        void IncDepth()
        {
            m_depth++;
        }

        void DecDepth()
        {
            m_depth--;
        }

        void Indent()
        {
            for( int i = 0; i < m_depth; i++ ){
                m_buffer += m_indetStr;
            }
        }

        void LineBreak()
        {
            m_buffer += "\n";
        }

        bool VisitEnter( const TiXmlDocument& )
        {
            return true;
        }

        bool VisitExit( const TiXmlDocument& )
        {
            return true;
        }

        bool VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute )
        {
            Indent();
            m_buffer += "<";
            m_buffer += element.Value();
#if 0
            for( const TiXmlAttribute* attrib = firstAttribute; attrib; attrib = attrib->Next() ){
                m_buffer += " ";
                attrib->Print( 0, 0, &m_buffer );
            }
#else
            IncDepth(); 
            for( const TiXmlAttribute* attrib = firstAttribute; attrib; attrib = attrib->Next() ){
                LineBreak();
                Indent();
                attrib->Print( 0, 0, &m_buffer );
            }
            DecDepth(); 
            if( firstAttribute ){
                LineBreak();
                Indent();
            }
#endif
            if( !element.FirstChild() ){
                m_buffer += " />";
                LineBreak();
            }
            else{
                m_buffer += ">";
                if( element.FirstChild()->ToText() &&
                    element.LastChild() == element.FirstChild() && 
                    element.FirstChild()->ToText()->CDATA() == false 
                ){
                        m_simpleTextPrint = true;
                }
                else{
                    LineBreak();
                }
            }
            IncDepth(); 
            return true;
        }


        bool VisitExit( const TiXmlElement& element )
        {
            DecDepth();
            if( !element.FirstChild() ){
                // nothing.
            }
            else{
                if( m_simpleTextPrint ){
                    m_simpleTextPrint = false;
                }
                else{
                    Indent();
                }
                m_buffer += "</";
                m_buffer += element.Value();
                m_buffer += ">";
                LineBreak();
            }
            return true;
        }


        bool Visit( const TiXmlText& text )
        {
            if( text.CDATA() ){
                Indent();
                m_buffer += "<![CDATA[";
                m_buffer += text.Value();
                m_buffer += "]]>";
                LineBreak();
            }
            else if( m_simpleTextPrint ){
                //TIXML_STRING str;
                //TiXmlBase::EncodeString( text.ValueTStr(), &str );

                // Text data is output as raw data.
                m_buffer += text.ValueTStr();
            }
            else{
                Indent();
                TIXML_STRING str;
                TiXmlBase::EncodeString( text.ValueTStr(), &str );
                m_buffer += str;
                LineBreak();
            }
            return true;
        }


        bool Visit( const TiXmlDeclaration& declaration )
        {
            Indent();
            declaration.Print( 0, 0, &m_buffer );
            LineBreak();
            return true;
        }


        bool Visit( const TiXmlComment& comment )
        {
            Indent();
            m_buffer += "<!--";
            m_buffer += comment.Value();
            m_buffer += "-->";
            LineBreak();
            return true;
        }


        bool Visit( const TiXmlUnknown& unknown )
        {
            Indent();
            m_buffer += "<";
            m_buffer += unknown.Value();
            m_buffer += ">";
            LineBreak();
            return true;
        }

    protected:
        TIXML_STRING m_buffer;
        TIXML_STRING m_indetStr;
        int m_depth;
        bool m_simpleTextPrint;

    };
};

#endif

