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

#ifndef ENV_PARAM_PARAM_DB_H
#define ENV_PARAM_PARAM_DB_H

#include "Env/Param/ParamXMLTree.h"

namespace Onikiri
{

    class ParamDB
    {
    protected:
        typedef ParamXMLTree::Node    XMLNode;
        typedef ParamXMLTree::NodePtr XMLNodePtr;
        typedef ParamXMLTree::NodeArray XMLNodeArray;
        typedef ParamXMLTree::ChildMap  XMLChildMap;
        typedef ParamXMLTree::AttributeMap XMLAttributeMap;

        String m_initialPath;
        ParamXMLTree m_tree;
        bool m_isLoaded;
        bool m_initialized;
        bool m_userParamPassed;

        String CompletePath( const String& target, const String& basePath );
        String RemoveFileName( const String& path );

        void AddExternalParameterToList( const String& name, const String& value );

        template <class T>
        String ToString( const ParamXMLPath& path, const T& param )
        {
            return boost::lexical_cast<std::string>(param);
        };

        template <class T>
        void ToParamRaw( T* dst, const ParamXMLPath& path, const String& str )
        {
            try{
                *dst = boost::lexical_cast<T>(str);
            }
            catch(boost::bad_lexical_cast& e){
                THROW_RUNTIME_ERROR(
                    "ParamDB parameter conversion failed.\n"
                    "Could not convert '%s' to user defined value.\n%s",
                    path.ToString().c_str(), e.what() 
                );
            }
        }

        template <class T>
        void ToParam( T* dst, const ParamXMLPath& path, const String& str )
        {
            boost::bad_lexical_cast orgExc;
            try{
                *dst = boost::lexical_cast<T>(str);
                return;
            }
            catch(boost::bad_lexical_cast& e){
                orgExc = e;
            }

            try{
                if(str.length() < 2){
                    throw orgExc;
                }

                char suffix = str.at(str.length() - 1);
                T base = 
                    boost::lexical_cast<T>( 
                        str.substr(0, str.length() - 1) );

                switch(suffix){
                case 'k': case 'K':
                    *dst = base*1000;
                    break;
                case 'm': case 'M':
                    *dst = base*1000000;
                    break;
                case 'g': case 'G':
                    *dst = base*1000000000;
                    break;
                default:
                    throw orgExc;
                    break;
                }

            }
            catch(boost::bad_lexical_cast&){
                THROW_RUNTIME_ERROR(
                    "ParamDB parameter conversion failed.\n"
                    "Could not convert '%s' to user defined value.\n%s",
                    path.ToString().c_str(), orgExc.what()
                );
            }
            
        };

        // Special versions
        // These types cannot be converted with a scale suffix such as 'k', 'm', and 'g'.
        void ToParam( bool* dst, const ParamXMLPath& path, const String& str );
        void ToParam( String* dst, const ParamXMLPath& path, const String& str );
        void ToParam( std::string* dst,  const ParamXMLPath& path, const String& str );


        void CheckResultXML( const String& path, XMLNodePtr node, int* warningCount );

        // Filter result XML tree recursively
        // return whether 'node' has valid children or not
        bool FilterResultXML( const String& path, XMLNodePtr node, const std::vector<String>& filterList );

        // Test whether the 'str' match one of the 'filterList'
        bool MatchFilterList( const String& str, const std::vector<String>& filterList ) const;


    public:
        ParamDB();
        ~ParamDB();

        bool Initialize( const String& initialPath );
        void Finalize();
        const std::vector<ParamXMLTree::InputInfo>& GetXMLFileNames();

        // Load XML parameter file.
        // If load failed, throw runtime_error.
        void LoadXMLFile(const String&);

        // Load parameters(command line arguments)
        void LoadParameters(const std::vector<String>&);

        // Add parameter
        // ( Argument is 'xpath=value' style.
        void AddParameter(const String& paramExp);
        
        // Add default param of user module
        void AddUserDefaultParam(const String& xmlString);

        // Dump result XML parameter
        String DumpResultXML( const String& level, const String& filter );

        // Get source file name of specified parameter.
        bool GetSourceXMLFile(const ParamXMLPath& parameterPath, String& sourceFile);

        // Get element count specified by a passed path
        size_t GetElementCount(const ParamXMLPath& path);

        // Get the XML tree
        ParamXMLTree& GetXMLTree();

        // For definition of data binding
        template <typename ValueType>
        struct Binding
        {
            const char* str;
            ValueType   value;
        };

        //
        // Setter
        //

        // For string
        void Set(const ParamXMLPath& path, const String&);

        // For generic type
        template <class T> 
        void Set(const ParamXMLPath& path, const T& param)
        {
            T orgParam = T();
            if( !Get( path, &orgParam ) || orgParam != param ){
                // Check the parameter is changed in this point,
                // because an internal expression in the XML tree and a value (T) are not equal
                // when the parameter has a special suffix (ex. 1K/1M)
                Set( path, ToString( path, param ) );
            }
        }

        // For generic array
        template <class T, int N> 
        void Set(const ParamXMLPath& path, const T (&val)[N])
        {
            String str;
            for(int i = 0; i < N; i++){
                if(i != 0) str += ",";
                str += ToString(path, val[i]);
            }
            Set( path, str );
        }

        // For vector
        template <class T> 
        void Set(const ParamXMLPath& path, const std::vector<T>& val )
        {
            String str;
            for(size_t i = 0; i < val.size(); i++){
                if(i != 0) str += ",";
                str += ToString(path, val[i]);
            }
            Set( path, str );
        }

        // For binded data
        template <class T> 
        void Set( const ParamXMLPath& path, const T& param, const Binding<T>* bindings, int bindingsSize )
        {
            String str;
            for( int i = 0; i < bindingsSize; i++ ){
                if( param == bindings[i].value ){
                    str = bindings[i].str;
                }
            }
            Set( path, str );
        }

        //
        // Getter
        //

        // For string
        bool Get(const ParamXMLPath& path, String* val, bool required = true);

        // For generic type
        template <class T> 
        bool Get(const ParamXMLPath& path, T* val, bool required = true)
        {
            String str;
            if( Get( path, &str, required ) ){
                ToParam( val, path, str );
                return true;
            }
            else{
                return false;
            }
        }

        // For array
        template < class T, int N > 
        bool Get(const ParamXMLPath& path, T (*val)[N], bool required = true)
        {
            String str;
            if( Get(path, &str, required) ){
                std::vector<String> valStrList = str.split(", \n\t");
                for(size_t i = 0; i < valStrList.size(); i++){
                    if(i >= N){
                        THROW_RUNTIME_ERROR(
                            "ParamDB parameter conversion failed.\n"
                            "'%s' is out of range.",
                            path.ToString().c_str() 
                        );
                    }
                    ToParam<T>( &val[i], path, valStrList[i] );
                }
                return true;
            }
            else{
                return false;
            }
        }

        // For vector
        template <class T> 
        bool Get(const ParamXMLPath& path, std::vector<T>* val, bool required = true)
        {
            String str;
            if( Get( path, &str, required ) ){
                std::vector<String> valList = str.split(", \n\t");
                val->resize( valList.size() );
                for(size_t i = 0; i < valList.size(); i++){
                    ToParam( &(*val)[i], path, valList[i] );
                }
                return true;
            }
            else{
                return false;
            }
        }

        // Binded data
        template <class T> 
        bool Get(const ParamXMLPath& path, T* val, const Binding<T>* bindings, int bindingsSize, bool required = true)
        {
            String str;
            if( Get(path, &str, required) ){
                bool converted = false;
                for( int i = 0; i < bindingsSize; i++ ){
                    if( str == bindings[i].str ){
                        *val = bindings[i].value;
                        converted = true;
                    }
                }
                if( !converted ){
                    String msg = "[";
                    for( int i = 0; i < bindingsSize; i++ ){
                        msg += bindings[i].str;
                        if( i < bindingsSize - 1 ){
                            msg += ", ";
                        }
                    }
                    msg += "]";         

                    THROW_RUNTIME_ERROR(
                        "'%s' is not a valid parameter. '%s' must be one of the following strings: %s",
                        str.c_str(),
                        path.ToString().c_str(),
                        msg.c_str()
                    );
                }
                return true;
            }
            else{
                return false;
            }
        }

    };


    // Global instance
    extern ParamDB g_paramDB;

} // namespace Onikiri

#endif // ENV_PARAM_PARAM_DB_H


