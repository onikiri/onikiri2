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


#ifndef SIM_DEPENDENCY_DEPENDENCY_H
#define SIM_DEPENDENCY_DEPENDENCY_H

#include "Types.h"

#include "Sim/Op/OpArray/OpArray.h"


namespace Onikiri {

    /* 
    依存関係を表現するための基底クラス
    */
    class Dependency 
    {
    public:
        typedef pool_list<OpIterator> ConsumerListType;
        typedef 
            ConsumerListType::iterator
            ConsumerListIterator;
        typedef 
            ConsumerListType::const_iterator
            ConsumerListConstIterator;


        Dependency();
        Dependency(int numScheduler);
        virtual ~Dependency();

        void AddConsumer( OpIterator op );
        void RemoveConsumer( OpIterator op );

        void Set();
        void Reset();

        void Clear();

        // accessors
        const ConsumerListType& GetConsumers() const
        {
            return m_consumer;
        }

        bool GetReadiness( const int index) const
        {
            return m_readiness.test( index );
        }

        bool IsFullyReady() const
        {
            for( size_t i = 0; i < m_readiness.size(); ++i ){
                if( !m_readiness.test(i) )
                    return false;
            }
            return true;
        }

        void SetReadiness( const bool readiness, const int index )
        {
            m_readiness.set( index, readiness );
        }

    protected:

        // スケジューラごとに ready かどうかのフラグ
        boost::dynamic_bitset<
            u32, boost::fast_pool_allocator<u32> 
        > m_readiness;  

        // 依存先の命令
        ConsumerListType    m_consumer;
    };

    enum DependencySetMaxSize { MAX_DEPENDENCY_SET_SIZE = 1024 };
    class DependencySet :
        public fixed_sized_buffer< Dependency*, MAX_DEPENDENCY_SET_SIZE, DependencySetMaxSize >
        //public pool_vector< Dependency* >
    {
    public:

        // Returns whether 'dep' is included in this set or not.
        bool IsIncluded( const Dependency* dep ) const
        {
            for( const_iterator i = begin(); i != end(); ++i ){
                if( *i == dep ){
                    return true;
                }
            }
            return false;
        }
    };

};

#endif // SIM_DEPENDENCY_DEPENDENCY_H

