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
// Parameter data exchange
//

#ifndef __ONIKIRI_PARAM_EXCHANGE_H
#define __ONIKIRI_PARAM_EXCHANGE_H

#include "Env/Param/ParamDB.h"
#include "Env/Env.h"

namespace Onikiri
{



/*
Typical usage scenario

<?xml version='1.0' encoding='UTF-8'?>
<Session>
    <Simulator>
        <Fetcher
            Width = "4"
        />
        <Core
            Param = "1"
        />
        <Core
            Param = "2"
        />
    </Simulator>
</Session>
----

class ParamTest : public ParamExchange
{
    int m_paramFetcher;
    int m_paramCore;
    int m_coreIndex;
    enum Policy 
    {
        POLICY_REFETCH,         
        POLICY_REISSUE_ALL,     
        POLICY_REISSUE_SELECTIVE,   
    };
    Policy m_policy;

public:
    ParamTest() 
    {   
        m_coreIndex = 0;
        LoadParam();    
    }
    ~ParamTest()
    {
        ReleaseParam(); 
    }

    BEGIN_PARAM_MAP("/Session/Simulator/")
        
        BEGIN_PARAM_PATH("Fetcher/")
            
            PARAM_ENTRY("@Width", m_paramFetcher)
        
            BEGIN_PARAM_BINDING(  "@Policy", m_policy, Policy )
                PARAM_BINDING_ENTRY( "Refetch",     POLICY_REFETCH )
                PARAM_BINDING_ENTRY( "ReissueAll",  POLICY_REISSUE_ALL )
                PARAM_BINDING_ENTRY( "ReissueSelective", POLICY_REISSUE_SELECTIVE )
            END_PARAM_BINDING()

        END_PARAM_PATH()

        BEGIN_PARAM_PATH_INDEX("Core", m_coreIndex)
            PARAM_ENTRY("@Param", m_paramCore)
        END_PARAM_PATH()

    END_PARAM_MAP()
};
*/


    #define BEGIN_PARAM_MAP( relativePath ) \
        void ProcessParamMap(bool save){ \
            const ParamXMLPath& basePath = GetRootPath() + relativePath; \

    #define BEGIN_PARAM_MAP_INDEX(relativePath, index) \
        void ProcessParamMap(bool save){ \
            const ParamXMLPath& basePath = \
                GetRootPath() + MakeIndexedPath( relativePath , index ); \

    #define END_PARAM_MAP() \
            basePath.begin(); /* For avoiding warning when the param map is empty and basePath is not touched. */ \
        } \

    #define BEGIN_PARAM_PATH( relativePath ) \
        {   \
            const ParamXMLPath& tmpPath  = basePath; \
            const ParamXMLPath& basePath = tmpPath + relativePath; \

    #define BEGIN_PARAM_PATH_INDEX(relativePath, index) \
        {   \
            const ParamXMLPath& tmpPath = basePath; \
            const ParamXMLPath& basePath( \
                tmpPath + MakeIndexedPath( relativePath , index ) ); \

    #define END_PARAM_PATH() \
            basePath.begin(); /* For avoiding warning when the param map is empty and basePath is not touched. */ \
        } \

    #define PARAM_ENTRY(relativePath, param) \
        { \
            ParamEntry((basePath), (relativePath), &(param), save); \
        } \

    #define RESULT_ENTRY(relativePath, param) \
        if(save){ \
            ResultEntry((basePath), (relativePath), (param)); \
        } \

    #define RESULT_RATE_ENTRY(relativePath, numerator, denominator) \
        if(save){ \
            ResultRateEntry((basePath), (relativePath), (numerator), (denominator)); \
        } \
    
    #define RESULT_RATE_SUM_ENTRY(relativePath, numerator, denominator1, denominator2) \
        if(save){ \
            ResultRateSumEntry((basePath), (relativePath), (numerator), (denominator1), (denominator2) ); \
        } \

    // For chaining to a object with a sub parameter path.
    #define CHAIN_PARAM_MAP(relativePath, param) \
        { \
            ChainParamMap((basePath), (relativePath), &param, save); \
        }

    // For chaining to a base class.
    #define CHAIN_BASE_PARAM_MAP(baseType) \
        { \
            baseType::ProcessParamMap(save); \
        }


    // For data binding
    #define BEGIN_PARAM_BINDING( relativePath, param, Type ) \
        { \
        Type& refParam = param; \
        const std::string& refRelativePath = relativePath; \
        typedef ParamDB::Binding<Type> Binding; \
        static const Binding table[] = \
            { \

    #define PARAM_BINDING_ENTRY( str, value ) \
                { (str), (value) },

    #define END_PARAM_BINDING() \
            }; \
            ParamBindingEntry( basePath, refRelativePath, &refParam, table, sizeof(table)/sizeof(Binding), save ); \
        }


class ParamExchangeBase
    {
    protected:
        ParamXMLPath m_rootPath;

    public:

        void SetRootPath(const ParamXMLPath& root)
        {
            m_rootPath = root;
        }

        const ParamXMLPath& GetRootPath()
        {
            return m_rootPath;
        }

        ParamXMLPath MakeIndexedPath(const String& base, int index)
        {
            return String(
                base + 
                "[" + boost::lexical_cast<String>(index) + "]/");
        }

        template <typename ValueT> 
        void ParamEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            ValueT* val, bool save)
        {
            if(save){
                g_paramDB.Set( basePath + relativePath, *val );
            }
            else{
                g_paramDB.Get( basePath + relativePath, val );
            }
        }

        template < typename ValueT > 
        void ParamBindingEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            ValueT* val,
            const ParamDB::Binding<ValueT>* bindings,
            int bindingsSize,
            bool save)
        {
            if(save){
                g_paramDB.Set(basePath + relativePath, *val, bindings, bindingsSize );
            }
            else{
                g_paramDB.Get(basePath + relativePath, val, bindings, bindingsSize );
            }
        }

        template <typename ValueT> 
        void ResultEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            const ValueT& val)
        {
            g_paramDB.Set( basePath + relativePath, val );
        }

        template <typename ValueT> 
        void ResultRateEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            const ValueT& numerator, const ValueT& denominator)
        {
            if( denominator != (ValueT)0 ){
                g_paramDB.Set( basePath + relativePath, (double)numerator / (double)denominator );
            }
            else{
                g_paramDB.Set( basePath + relativePath, 0 );
            }
        }

        template <typename ValueT> 
        void ResultRateEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            const std::vector<ValueT>& n, const std::vector<ValueT>& d)
        {
            std::vector<double> rate;
            size_t size = d.size();
            rate.resize( size );
            for( size_t i = 0; i < size; i++){
                if( d[i] != (ValueT)0 ){
                    rate[i] = (double)n[i] / (double)d[i];
                }
                else{
                    rate[i] = 0;
                }
            }
            g_paramDB.Set( basePath + relativePath, rate );
        }

        template <typename ValueT> 
        void ResultRateSumEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            const ValueT& numerator, 
            const ValueT& denominator1,
            const ValueT& denominator2 )
        {
            ResultRateEntry( basePath, relativePath, numerator, denominator1 + denominator2 );
        }

        template <typename ValueT> 
        void ResultRateSumEntry(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            const std::vector<ValueT>& n,
            const std::vector<ValueT>& d1,
            const std::vector<ValueT>& d2 )
        {
            std::vector<ValueT> d;
            size_t size = std::min( d1.size(), d2.size() );
            d.resize( size );
            for( size_t i = 0; i < size; i++){
                d[i] = d1[i] + d2[i];
            }
            ResultRateEntry( basePath, relativePath, n, d );
        }

        template <typename T>
        void ChainParamMap(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            T* param, 
            bool save 
        ){
            param->SetRootPath( (basePath) + (relativePath) );
            param->ProcessParamMap( save );
        }

        template <typename T>
        void ChainParamMap(
            const ParamXMLPath& basePath, 
            const ParamXMLPath& relativePath, 
            std::vector<T>* param, 
            bool save
        ){
            size_t count = g_paramDB.GetElementCount( basePath + relativePath );
            if(param->size() < count) 
                param->resize(count);

            for(size_t i = 0; i < param->size(); i++){
                (*param)[i].SetRootPath(
                    MakeIndexedPath((basePath + relativePath).ToString(), (int)i) );
                (*param)[i].ProcessParamMap(save);
            }

        }

        virtual void ProcessParamMap(bool save) = 0;

        ParamExchangeBase()
        {
        };

        virtual ~ParamExchangeBase()
        {
        };
    };
    
    class ParamExchange : public ParamExchangeBase
    {
    private:
        bool m_released;
    public:
        ParamExchange();
        virtual ~ParamExchange();

        virtual void LoadParam();
        virtual void ReleaseParam();
        virtual bool IsParameterReleased();
    }; // class ParamExchange

    class ParamExchangeChild : public ParamExchangeBase
    {
    public:
        const ParamXMLPath& GetChildRootPath();
        void SetRootPath(const ParamXMLPath& root);
    };

}   // namespace Onikiri

#endif


