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


#ifndef SIM_MEMORY_CACHE_CACHE_EXTRA_STATE_TABLE_H
#define SIM_MEMORY_CACHE_CACHE_EXTRA_STATE_TABLE_H

#include "Sim/Memory/Cache/Cache.h"

namespace Onikiri
{

    //
    // A mechanism adding extra information to cache lines.
    // Information corresponding to a line that is pointed by setassoc_table::iterator
    // is get from / set to this table.
    // Cache/PrefetcherBase classes use this table and they are examples of usage.
    //

    class Cache;

    template < typename ValueType, typename ContainerType = std::vector<ValueType> >
    class CacheExtraStateTable
    {

    public:
        typedef typename ContainerType::reference ReferenceType;
        typedef typename ContainerType::const_reference ConstReferenceType;
        typedef typename CacheTable::iterator iterator;
        typedef typename CacheTable::const_iterator const_iterator;

    protected:
        ContainerType m_table;
        size_t m_indexCount;
        size_t m_wayCount;

        template < typename T >
        size_t GetTableIndex( const T& i ) const
        {
            ASSERT(
                m_wayCount > 0 && m_indexCount > 0, 
                "CacheExtraStateTable is not initialized." 
            );
            ASSERT( 
                i.way() < m_wayCount && i.index() < m_indexCount,
                "The iterator points out of range."
            );
            return i.way() + i.index() * m_wayCount;
        }

    public:
        void Resize( Cache* cache, const ValueType& initValue = ValueType() )
        {
            m_indexCount = cache->GetIndexCount();
            m_wayCount   = cache->GetWayCount();
            m_table.resize( m_indexCount * m_wayCount, initValue );
        }

        CacheExtraStateTable( Cache* cache )
        {
            Resize( cache );
        }

        CacheExtraStateTable() :
            m_indexCount(0),
            m_wayCount(0)
        {
        }


        ReferenceType operator[]( const iterator& i )
        {
            return m_table[ GetTableIndex(i) ];
        }

        ConstReferenceType operator[]( const iterator& i ) const 
        {
            return m_table[ GetTableIndex(i) ];
        }

        ReferenceType operator[]( const const_iterator& i )
        {
            return m_table[ GetTableIndex(i) ];
        }

        ConstReferenceType operator[]( const const_iterator& i ) const 
        {
            return m_table[ GetTableIndex(i) ];
        }

    };

}; // namespace Onikiri

#endif // SIM_MEMORY_CACHE_CACHE_EXTRA_STATE_TABLE_H

