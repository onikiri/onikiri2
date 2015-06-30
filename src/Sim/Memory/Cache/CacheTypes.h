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


#ifndef SIM_MEMORY_CACHE_CACHE_TYPES_H
#define SIM_MEMORY_CACHE_CACHE_TYPES_H

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Pipeline/PipelineNodeIF.h"
#include "Sim/Memory/AddrHasher.h"

namespace Onikiri
{
    class Cache;

    struct CacheLineValue
    {
        // u8 value[64]
    };  

    typedef
        AdrHasher 
        CacheHasher;

    typedef
        std::pair<Addr, CacheLineValue> 
        CachePair;

    typedef 
        shttl::setassoc_table<
            CachePair, 
            CacheHasher,
            shttl::lru_list< Addr, u8 >
        >
        CacheTable;

/* For caches with large associativity.
    typedef 
        shttl::setassoc_table_strage_map< 
            shttl::setassoc_table_line_base< CachePair >
        >
        CacheStrage;
    typedef 
        shttl::setassoc_table<
            CachePair, 
            CacheHasher,
            shttl::lru_list< Addr, u32 >,
            CacheStrage
        >
        CacheTable;
*/

    typedef 
        CacheTable::line_type 
        CacheLine;

    typedef 
        CacheTable::iterator 
        CacheTableIterator;

    typedef 
        CacheTable::const_iterator 
        CacheTableConstIterator;

    // Additional information for cache accesses.
    struct CacheAccess : public MemAccess
    {
        // Cache operation type.
        enum OperationType
        {
            OT_READ,
            OT_WRITE,
            OT_WRITE_BACK,
            OT_READ_FOR_WRITE_ALLOCATE,
            OT_PREFETCH
        };
        OperationType  type;
        OpIterator     op;
        CacheLineValue lineValue;

        CacheAccess(
            const MemAccess& newMem   = MemAccess(), 
            OpIterator       op       = OpIterator(),
            OperationType    newType  = OT_READ,
            CacheLineValue   newLineValue = CacheLineValue()
        ) : 
            MemAccess( newMem ),
            type ( newType ),
            op   ( op ),
            lineValue( newLineValue )
        {
        }

        bool IsWirte() const
        {
            return type == OT_WRITE || type == OT_WRITE_BACK;
        }
    };

    // Cache access result.
    // Read/Write methods returns this structure.
    struct CacheAccessResult
    {
        int  latency;
        enum State
        {
            ST_HIT,
            ST_PENDING_HIT,
            ST_MISS,
            ST_NOT_ACCESSED
        };
        State  state;
        Cache* cache; 

        // 'line' has an iterator of an accessed line on Read/Write.
        // On 'Write', this 'line' also indicates a replaced line.
        CacheTableIterator line;

        CacheAccessResult( 
            int     newLatency = -1, 
            State   newState = ST_NOT_ACCESSED, 
            Cache*  newCache = NULL,
            CacheTableIterator newLine = CacheTableIterator()
        ) :
            latency( newLatency ),
            state  ( newState ),
            cache  ( newCache ),
            line   ( newLine )
        {
        }
    };

    enum CacheAccessEventType
    {
        // ミス時の高次キャッシュへのアクセスが帰ってきた際に，PendingAccess から呼ばれる．
        // AccessFinished の実装では addr に該当するラインへ書き込みを反映させる．
        CAET_FILL_FROM_NEXT_CACHE_FINISHED,     
        CAET_FILL_FROM_MAL_FINISHED,        // From a missed access list

        // The end of accesses
        CAET_WRITE_ALLOCATE_FINISHED,
        CAET_WRITE_ACCESS_FINISHED,
        CAET_READ_ACCESS_FINISHED
    };

    struct CacheAccessNotificationParam
    {
        CacheAccessEventType type;
        CacheAccessResult    result;

        CacheAccessNotificationParam(
            const CacheAccessEventType newType = CAET_FILL_FROM_NEXT_CACHE_FINISHED,
            const CacheAccessResult&   newResult = CacheAccessResult()
        ) :
            type( newType ),
            result( newResult )
        {
        }
    };

    class CacheAccessNotifieeIF
    {
    public:
        virtual ~CacheAccessNotifieeIF(){};

        // PendingAccess から各種アクセス終了の通知をうける
        virtual void AccessFinished( 
            const CacheAccess&     access, 
            const CacheAccessNotificationParam &param 
        ) = 0;
    };

    // A hook parameter
    struct CacheHookParam
    {
        const CacheAccess*      access;
        const Addr*             address;
        const CacheTable*       table;
        CacheAccessNotifieeIF*  notifiee;
        CacheAccessResult       result;
        CacheTableConstIterator line;       // A iterator of a touched/replaced line.
        bool replaced;
        CacheHookParam() : 
            access  ( NULL ),
            address    ( NULL ),
            table   ( NULL ),
            notifiee( NULL ),
            replaced( false )
        {
        }
    };

}; // namespace Onikiri

#endif // __CACHEIF_H__

