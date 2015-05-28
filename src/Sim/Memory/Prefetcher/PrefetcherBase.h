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
// A base class of prefetchers.
// Users can implement an original prefetcher by 
// implementing 'OnCacheAccess' of a class inherited from this.
//

#ifndef SIM_MEMORY_PREFETCHER_PREFETCHER_BASE_H
#define SIM_MEMORY_PREFETCHER_PREFETCHER_BASE_H

#include "Sim/Memory/Prefetcher/PrefetcherIF.h"
#include "Sim/Memory/Cache/CacheExtraStateTable.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri
{
    class Cache;

    class PrefetcherBase :
        public PrefetcherIF, 
        public CacheAccessNotifieeIF,
        public PhysicalResourceNode
    {
    public:

        PrefetcherBase();
        virtual ~PrefetcherBase();

        // --- PrefetcherIF

        // --- PhysicalResourceNode
        virtual void Initialize( InitPhase phase );

        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@EnablePrefetch",  m_enabled );
                PARAM_ENTRY( "@Name", m_name );
                PARAM_ENTRY( "@OffsetBitSize",   m_lineBitSize );
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumPrefetch",        m_numPrefetch );
                PARAM_ENTRY( "@NumReadHitAccess",   m_numReadHitAccess );
                PARAM_ENTRY( "@NumReadMissAccess",  m_numReadMissAccess );
                PARAM_ENTRY( "@NumWriteHitAccess",  m_numWriteHitAccess );
                PARAM_ENTRY( "@NumWriteMissAccess", m_numWriteMissAccess );
                PARAM_ENTRY( "@NumReplacedLine",    m_numReplacedLine );
                PARAM_ENTRY( "@NumPrefetchedReplacedLine",  m_numPrefetchedReplacedLine );
                PARAM_ENTRY( "@NumEffectivePrefetchedLine", m_numEffectivePrefetchedReplacedLine );
                RESULT_RATE_ENTRY( 
                    "@NumAccuracy",
                    m_numEffectivePrefetchedReplacedLine , 
                    m_numPrefetchedReplacedLine 
                );
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Cache, "target",   m_prefetchTarget )
        END_RESOURCE_MAP()

        virtual void ChangeSimulationMode( SimulationMode mode );

        //
        // --- PrefetcherIF
        //

        // Cache read
        virtual void OnCacheRead( Cache* cache, CacheHookParam* param );

        // Cache write
        virtual void OnCacheWrite( Cache* cache, CacheHookParam* param );

        // Invalidate line
        virtual void OnCacheInvalidation( Cache* cache, CacheHookParam* param );

        // Write an access to the cache table.
        virtual void OnCacheTableUpdate( Cache* cache, CacheHookParam* param );

        //
        //--- CacheAccessNotifieeIF
        // 

        // PendingAccess から各種アクセス終了の通知をうける
        virtual void AccessFinished( 
            const CacheAccess& access, 
            const CacheAccessNotificationParam& param 
        );

        //
        // ---
        //

        // This method is called only when a cache access is not a prefetch access.
        // Each prefetch algorithm is implemented in this method of a inherited class.
        virtual void OnCacheAccess( 
            Cache* cache, const CacheAccess& access, bool hit
        ) = 0;

    protected:
        static const size_t MAX_INFLIGHT_PREFETCH_ACCESSES = 256;
        
        String m_name;

        // A prefetch target cache.
        // A cache line is prefetched from a next cache of this cache to this cache.
        Cache* m_prefetchTarget;

        bool m_enabled;

        // Parameters of a cache line
        int m_lineSize;
        int m_lineBitSize;

        // Statistics
        s64 m_numPrefetch;

        s64 m_numReadHitAccess;
        s64 m_numReadMissAccess;
        s64 m_numWriteHitAccess;
        s64 m_numWriteMissAccess;

        // The number of replaced lines
        s64 m_numReplacedLine;           
        
        // The number of replaced lines that are prefetched.
        s64 m_numPrefetchedReplacedLine; 
        
        // The number of replaced lines that are prefetched 
        // and are actually accessed.
        s64 m_numEffectivePrefetchedReplacedLine;   

        // A current simulation mode
        SimulationMode m_mode;

        // An access list of in-flight prefetch access.
        struct PrefetchAccess : public CacheAccess
        {
            // The number of accesses that access this prefetching line.
            int  accessdCount; 
            bool invalidated;

            PrefetchAccess( const CacheAccess& cacheAccess = CacheAccess() ) :
                CacheAccess( cacheAccess ),
                accessdCount( 0 ),
                invalidated ( false )
            {
            }
        };
        typedef pool_list< PrefetchAccess > AccessList;
        AccessList m_accessList;
        AccessList::iterator FindPrefetching( const Addr& addr );

        // An extra state of an cache line
        struct ExLineState
        {
            bool valid;
            bool prefetched;
            bool accessed;
            ExLineState() :
                valid     ( false ),
                prefetched( false ),
                accessed  ( false )
            {
            }
        };
        CacheExtraStateTable< ExLineState > m_exLineState;

        // Update cache access statistics.
        virtual void UpdateCacheAccessStat( const CacheAccess& access, bool hit );

        // Increment the number of prefetch.
        virtual void IncrementPrefetchNum();

        // Masks offset bits of the cache line in the 'addr'.
        u64 MaskLineOffset( u64 addr );

        // Returns whether an access is prefetch or not.
        bool IsPrefetch( const CacheAccess& access );

        // Invoke a prefetch access.
        void Prefetch( const CacheAccess& access );

        // Returns whether this access is from this prefetcher or not.
        bool IsAccessFromThisPrefetcher( CacheHookParam* param ) const;
    };


}; // namespace Onikiri

#endif // SIM_MEMORY_PREFETCHER_PREFETCHER_BASE_H

