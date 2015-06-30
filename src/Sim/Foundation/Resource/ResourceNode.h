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


#ifndef SIM_FOUNDATION_RESOUCE_RESOURCE_NODE_H
#define SIM_FOUNDATION_RESOUCE_RESOURCE_NODE_H

#include "Types.h"
#include "Sim/Foundation/Resource/ResourceBase.h"
#include "Sim/Foundation/Resource/ResourceArray.h"
#include "Env/Param/ParamExchange.h"

namespace Onikiri 
{
    //
    // --- Macros for resource map 
    //

    struct ResourceConnectionResult
    {
        int connectedCount;
        int entryCount;
        ResourceConnectionResult() :
            connectedCount(0),
            entryCount(0)
        {
        }

        void Add( const ResourceConnectionResult& rhs )
        {
            connectedCount  += rhs.connectedCount;
            entryCount      += rhs.entryCount;
        }
    };

    #define BEGIN_RESOURCE_MAP() \
        ResourceConnectionResult \
            ConnectResource( PhysicalResourceBaseArray& srcArray, const String& srcName, const String& to, bool chained ) \
        { \
            return ConnectResourceBody( *this, srcArray, srcName, to, chained ); \
        } \
        template <typename ClassType> \
        ResourceConnectionResult \
            ConnectResourceBody( ClassType& refThis, PhysicalResourceBaseArray& srcArray, const String& srcName, const String& to, bool chained ) \
        { \
            ResourceConnectionResult result; \

    #define END_RESOURCE_MAP() \
            if(!chained) \
                CheckConnection( srcArray, to, result ); \
            return result;\
        }

    #define RESOURCE_OPTIONAL_ENTRY( typeName, dstName, resEntry ) \
        ConnectResourceEntry<typeName>( resEntry, srcArray, #typeName, dstName, srcName, to, &result ); \

    #define RESOURCE_ENTRY( typeName, dstName, resEntry ) \
        RESOURCE_OPTIONAL_ENTRY( typeName, dstName, resEntry ) \
        result.entryCount++;

    #define CHAIN_BASE_RESOURCE_MAP( classType ) \
        result.Add( classType::ConnectResource( srcArray, srcName, to, true ) );

    // Setter prototype : 
    // setter( const PhysicalResourceArray< ValueType >& array );
    // setter( const ValueType* scalar );
    #define RESOURCE_OPTIONAL_SETTER_ENTRY( typeName, dstName, setter ) \
        ConnectResourceEntry<ClassType, typeName>( &ClassType::setter, srcArray, #typeName, dstName, srcName, to, &result ); \

    #define RESOURCE_SETTER_ENTRY( typeName, dstName, setter ) \
        RESOURCE_OPTIONAL_SETTER_ENTRY( typeName, dstName, setter ) \
        result.entryCount++;


    // This class is an interface for dynamic_cast with a type name string 'typeName'.
    // We can not use original 'dynamic_cast' in this situation,  
    // because the dynamic_cast can not cast an incomplete type declared by forward declaration.
    class ResourceTypeConverterIF
    {
    public:
        virtual ~ResourceTypeConverterIF(){};
        virtual void* DynamicCast(const String& typeName, PhysicalResourceIF* orgPtr) = 0;
    };

    // Node information 
    struct PhysicalResourceNodeInfo
    {
        String typeName;
        String name;
        ParamXMLPath paramPath;
        ParamXMLPath resultRootPath;
        ParamXMLPath resultPath;
    };

    // A base class for a physical resource node of the 'ResourceBuilder' system.
    class PhysicalResourceNode : 
        public PhysicalResourceBase,
        public ParamExchange
    {
    protected:
        bool m_initialized;
        PhysicalResourceNodeInfo m_info;
        ResourceTypeConverterIF* m_typeConverter;
        String m_who;
        int m_totalEntryCount;
        int m_connectedEntryCount;

        typedef
            PhysicalResourceArray<PhysicalResourceNode>
            PhysicalResourceBaseArray;

        // Copy the instances of the type 'typeName' from 'srcArray' to 'resArray'.
        template <typename ArrayValueType>
        void CopyResourceArray(
            const String& typeName,
            PhysicalResourceArray<ArrayValueType>& resArray,
            PhysicalResourceBaseArray& srcArray )
        {
            resArray.Resize( srcArray.GetSize() );
            for( int i = 0; i < srcArray.GetSize(); i++ ){
                ArrayValueType* res;
                DynamicCast( &res, typeName, srcArray[i] );
                if(!res){
                    THROW_RUNTIME_ERROR( "dynamic cast failed." );
                }

                resArray[i] = res;
            }
        }

        // Connect resources
        // This method is used for connecting an array.
        template <typename ArrayValueType>
        void ConnectResourceEntry( 
            PhysicalResourceArray<ArrayValueType>& resArray,
            PhysicalResourceBaseArray& srcArray, 
            const char* typeName,
            const char* dstName,
            const String& srcName,
            const String& to,
            ResourceConnectionResult* result 
        ){
            ASSERT( srcArray.GetSize() > 0 );

            ArrayValueType* res;
            DynamicCast( &res, typeName, srcArray[0] );

            // Test dynamic cast and names
            if( res && ( (to == "" && srcName == dstName ) || to == dstName ) ){
                CopyResourceArray( typeName, resArray, srcArray );
                result->connectedCount++;
            }
        }

        // Connect resource
        // This method is used for connecting scalar.
        template <typename ArrayValueType>
        void ConnectResourceEntry( 
            ArrayValueType*& resEntry,
            PhysicalResourceBaseArray& srcArray, 
            const char* typeName,
            const char* dstName,
            const String& srcName,
            const String& to,
            ResourceConnectionResult* result 
        ){
            ASSERT( srcArray.GetSize() > 0 );

            ArrayValueType* res;
            DynamicCast( &res, typeName, srcArray[0] );

            // Test dynamic cast and names
            if( res && ( (to == "" && srcName == dstName ) || to == dstName ) ){
                CheckNodeIsScalar( srcArray, dstName, srcName );
                resEntry = res;
                result->connectedCount++;
            }
        }

        // Connect resources by using the setter method
        template <typename ClassType, typename ArrayValueType>
        void ConnectResourceEntry( 
            void (ClassType::*setter)( PhysicalResourceArray<ArrayValueType>& ),
            PhysicalResourceBaseArray& srcArray, 
            const char* typeName,
            const char* dstName,
            const String& srcName,
            const String& to, 
            ResourceConnectionResult* result 
        ){
            ASSERT( srcArray.GetSize() > 0 );

            ArrayValueType* res;
            DynamicCast( &res, typeName, srcArray[0] );

            // Test dynamic cast and names
            if( res && ( (to == "" && srcName == dstName ) || to == dstName ) ){

                PhysicalResourceArray<ArrayValueType> resArray;
                CopyResourceArray( typeName, resArray, srcArray );

                // Call the setter by using the pointer of the derived class.
                (dynamic_cast<ClassType*>(this)->*setter)( resArray );

                result->connectedCount++;
            }
        }

        // Connect resources by using the setter method
        template <typename ClassType, typename ArrayValueType>
        void ConnectResourceEntry( 
            void (ClassType::*setter)( ArrayValueType* ),
            PhysicalResourceBaseArray& srcArray, 
            const char* typeName,
            const char* dstName,
            const String& srcName,
            const String& to, 
            ResourceConnectionResult* result 
        ){
            ASSERT( srcArray.GetSize() > 0 );

            ArrayValueType* res;
            DynamicCast( &res, typeName, srcArray[0] );

            // Test dynamic cast and names
            if( res && ( (to == "" && srcName == dstName ) || to == dstName ) ){
                CheckNodeIsScalar( srcArray, dstName, srcName );
                // Call the setter by using the pointer of the derived class.
                (dynamic_cast<ClassType*>(this)->*setter)( res );   
                result->connectedCount++;
            }
        }

        // Dynamic cast wrapper
        template <typename T>
        void DynamicCast( 
            T**ptr,
            const String& typeName,
            PhysicalResourceNode* orgPtr )
        {
            if(!m_typeConverter){
                THROW_RUNTIME_ERROR( "A resource type converter is not set." );
            }

            *ptr = 
                static_cast<T*>(
                    m_typeConverter->DynamicCast( typeName, orgPtr )
                );
        };

        void CheckConnection(
            PhysicalResourceBaseArray& res,
            const String& to,
            const ResourceConnectionResult& result 
        );
        
        void CheckNodeIsScalar( 
            PhysicalResourceBaseArray& srcArray,
            const String& dstName,
            const String& srcName
        );

        template <class T>
        void CheckNodeInitialized( 
            const String& nodeName,
            T* resPtr
        ){
            if( resPtr == 0 ){
                THROW_RUNTIME_ERROR(
                    "'%s.%s' is not set.",
                    m_info.name.c_str(),
                    nodeName.c_str()
                );
            }
        }

        template <class T>
        void CheckNodeInitialized( 
            const String& nodeName,
            PhysicalResourceArray<T>& resArray
        ){
            if( resArray.GetSize() == 0 ){
                THROW_RUNTIME_ERROR(
                    "'%s.%s' is not set.",
                    m_info.name.c_str(),
                    nodeName.c_str()
                );
            }
        }

        BEGIN_PARAM_MAP( "" )
            BEGIN_PARAM_PATH( GetResultRootPath() )
                PARAM_ENTRY( "@Name", m_info.name )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@RID",  m_rid );
            END_PARAM_PATH()
        END_PARAM_MAP()

    public:
        PhysicalResourceNode();
        ~PhysicalResourceNode();

        void SetInfo( const PhysicalResourceNodeInfo& info );
        const PhysicalResourceNodeInfo& GetInfo();
        
        const char* Who() const;

        const String& GetName() const;
        const String& GetTypeName() const;
        const ParamXMLPath& GetParamPath() const;
        const ParamXMLPath& GetResultPath() const;
        const ParamXMLPath& GetResultRootPath() const;

        void SetTypeConverter( ResourceTypeConverterIF* );

        void ReleaseParam();

        // This method must be defined in the BEGIN_RESOURCE_MAP macro.
        virtual ResourceConnectionResult ConnectResource(
            PhysicalResourceBaseArray& srcArray, 
            const String& srcName, 
            const String& to,
            bool chained
        );

        // Initializing phase
        enum InitPhase
        {
            // After constructing and before object connection.
            // ParamExchange::LoadParam() must be called in this phase or later.
            INIT_PRE_CONNECTION,    

            // After connection
            INIT_POST_CONNECTION
        };

        virtual void Initialize(InitPhase phase) = 0;

        // This method is called just before all nodes are destructed.
        // In this timing, all nodes are still alive.
        virtual void Finalize(){};

        // This method is called when a simulation mode is changed.
        enum SimulationMode
        {
            SM_EMULATION,
            SM_INORDER,
            SM_SIMULATION
        };
        virtual void ChangeSimulationMode( SimulationMode mode ){};

        // Validate connection.
        void ValidateConnection();
    };

}; // namespace Onikiri

#endif // SIM_FOUNDATION_RESOUCE_RESOURCE_NODE_H


