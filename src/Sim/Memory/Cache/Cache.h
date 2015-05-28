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
// A cache class.
//

#ifndef SIM_MEMORY_CACHE_CACHE_H
#define SIM_MEMORY_CACHE_CACHE_H

#include "Env/Param/ParamExchange.h"

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Memory/Cache/CacheTypes.h"
#include "Sim/Pipeline/PipelineNodeBase.h"
#include "Sim/Foundation/Hook/HookDecl.h"

namespace Onikiri
{
    class CacheMissedAccessList;
    class CacheAccessRequestQueue;
    class Core;
    class PrefetcherIF;
    template < 
        typename ValueType, 
        typename ContainerType
    > class CacheExtraStateTable;

    // 'Cache' is a pipeline node and runs independently from instruction pipelines.
    class Cache :
        public CacheAccessNotifieeIF,
        public PipelineNodeBase
    {
    public:
        typedef size_t  SizeType;
        typedef CacheAccess            Access;
        typedef CacheAccessResult      Result;
        typedef CacheAccessNotificationParam NotifyParam;
    
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY("@Name",                m_name);
                PARAM_ENTRY("@Latency",             m_latency);
                PARAM_ENTRY("@Perfect",             m_perfect);
                PARAM_ENTRY("@IndexBitSize",        m_indexBitSize);
                PARAM_ENTRY("@OffsetBitSize",       m_offsetBitSize);
                PARAM_ENTRY("@NumWays",             m_numWays);
                PARAM_ENTRY("@NumPorts",            m_numPorts);
                PARAM_ENTRY("@RequestQueueSize",        m_reqQueueSize);
                PARAM_ENTRY("@MissedAccessListSize",    m_missedAccessListSize);
                PARAM_ENTRY("@ExclusiveAccessCycles",   m_exclusiveAccessCycles);
                BEGIN_PARAM_BINDING( "@WritePolicy", m_writePolicy, WritePolicy )
                    PARAM_BINDING_ENTRY( "WriteThrough", WP_WRITE_THROUGH )
                    PARAM_BINDING_ENTRY( "WriteBack",    WP_WRITE_BACK )
                END_PARAM_BINDING()
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY("@TableName",           m_name);
                PARAM_ENTRY("@NumReadHit",          m_numReadHit);
                PARAM_ENTRY("@NumReadMiss",         m_numReadMiss);
                PARAM_ENTRY("@NumReadAccess",       m_numReadAccess);
                PARAM_ENTRY("@NumReadPendingHit",   m_numReadPendingHit);
                PARAM_ENTRY("@NumPrefetchHit",      m_numPrefetchHit);
                PARAM_ENTRY("@NumPrefetchMiss",     m_numPrefetchMiss);
                PARAM_ENTRY("@NumPrefetchPendingHit",   m_numPrefetchPendingHit);
                PARAM_ENTRY("@NumPrefetchAccess",   m_numPrefetchAccess);
                PARAM_ENTRY("@NumWriteHit",         m_numWriteHit);
                PARAM_ENTRY("@NumWriteMiss",        m_numWriteMiss);
                PARAM_ENTRY("@NumWriteAccess",      m_numWriteAccess);
                PARAM_ENTRY("@NumWritePendingHit",  m_numWritePendingHit);
                PARAM_ENTRY("@NumWrittenBackLines", m_numWrittenBackLines);
                PARAM_ENTRY("@NumInvalidatedLines", m_numInvalidated);
                PARAM_ENTRY("@CapacityKByte",       m_capacityKB);
                PARAM_ENTRY("@MaxThroughputBytesPerCycle",  m_maxThroughputBytesPerCycle );
                RESULT_RATE_ENTRY("@NumReadHitRate",  m_numReadHit,  m_numReadAccess )
                RESULT_RATE_ENTRY("@NumWriteHitRate", m_numWriteHit, m_numWriteAccess )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core,   "core",     m_core )
            RESOURCE_ENTRY( Thread, "thread",   m_thread )
            RESOURCE_OPTIONAL_ENTRY(        PrefetcherIF,   "prefetcher",   m_prefetcher )
            RESOURCE_OPTIONAL_SETTER_ENTRY( Cache,          "cache",        SetNextCache )
        END_RESOURCE_MAP()

        Cache();
        virtual ~Cache();

        //
        // --- PhysicalResourceNode
        //
        
        virtual void Initialize(InitPhase phase);

        // This method is called when a simulation mode is changed.
        virtual void ChangeSimulationMode( PhysicalResourceNode::SimulationMode mode );

        //
        // --- PendingAccessNotifeeIF
        //

        // PendingAccess から各種アクセス終了の通知をうける
        void AccessFinished( const Access& addr, const NotifyParam& param );

        //
        // --- Cache
        //

        // perfect かどうかを返す
        bool IsPerfect() { return m_perfect > 0; }
        
        // ヒットした時のレイテンシを返す
        const int GetStaticLatency() const { return m_latency; }
        
        // リードしてレイテンシを返す
        Result Read( const Access& access, CacheAccessNotifieeIF* notifiee );
        
        // ライトしてレイテンシを返す
        // レイテンシは，ライトミス時にライトアロケートにかかった時間を含む
        Result Write( const Access& access, CacheAccessNotifieeIF* notifiee );
        
        // Invalidate line
        void Invalidate( const Addr& addr );

        // キャッシュのラインサイズをオフセットのビット数で返す
        int GetOffsetBitSize() const;

        // 次のレベルのCacheへのポインタを返す
        Cache* GetNextCache();

        // 次のレベルのCacheをセットする
        void SetNextCache( PhysicalResourceArray<Cache>& next );

        // Add previous level cache
        void AddPreviousLevelCache( Cache* prev );

        // Returns true when resources (a request queue etc) are full and stall is required.
        bool IsStallRequired();

        int GetIndexCount() { return 1 << m_indexBitSize; }
        int GetWayCount()   { return m_numWays;           }

        //
        // --- PipelineNodeIF/PipelineNodeBase
        //

        // シミュレーションの本体
        virtual void Update();


        //
        // --- Hooks
        // The all hooks can register methods with the following style:
        // void Method( HookParam* param );

        // A hook for Read
        static HookPoint<Cache, CacheHookParam> s_readHook; 

        // A hook for Write 
        static HookPoint<Cache, CacheHookParam> s_writeHook;    

        // A hook for Invalidate
        static HookPoint<Cache, CacheHookParam> s_invalidateHook;   

        // A hook for UpdateTable
        // UpdateTable is called just when the table(setassoc_table) is written on
        // re-fill, write and write-back.
        static HookPoint<Cache, CacheHookParam> s_tableUpdateHook;  

    protected:

        typedef CacheLine   Line;
        typedef CacheLineValue  Value;
        typedef CacheHasher HasherType;
        typedef CachePair   PairType;
        typedef CacheTable  TableType;

        typedef Result::State ResultState;
        typedef Access::OperationType AccessType;
        typedef CacheAccessNotifieeIF NotifieeIF;

        static const AccessType AOT_READ     = Access::OT_READ;
        static const AccessType AOT_WRITE    = Access::OT_WRITE;
        static const AccessType AOT_WRITE_BACK = Access::OT_WRITE_BACK;
        static const AccessType AOT_READ_FOR_WRITE_ALLOCATE = Access::OT_READ_FOR_WRITE_ALLOCATE;
        static const AccessType AOT_PREFETCH = Access::OT_PREFETCH;

        int m_latency;
        TableType* m_cacheTable;

        // Line state table
        struct LineState
        {
            bool dirty;
        };
        typedef CacheExtraStateTable< LineState, std::vector<LineState> > ExtraStateTableType;
        ExtraStateTableType* m_lineState;

        // 次のLevelのキャッシュ・メモリ
        Cache*  m_nextLevelCache;

        // Previous level caches
        PhysicalResourceArray<Cache> m_prevLevelCaches;

        // プリフェッチャ
        PrefetcherIF* m_prefetcher;

        int m_perfect;

        // Throughput of this cache specified by bytes per cycle.
        // This is determined by the following:
        //   (line size) / m_exclusiveAccessCycles.
        String m_maxThroughputBytesPerCycle;

        // One memory access exclusively use a memory/cache.
        int m_exclusiveAccessCycles;

        // Write policy
        enum WritePolicy
        {
            WP_INVALID,
            WP_WRITE_THROUGH,
            WP_WRITE_BACK
        };
        WritePolicy m_writePolicy;

        // A list of memory accesses that access this cache/memory layer.
        CacheMissedAccessList* m_missedAccessList;      

        // A queue of cache access requests.
        // This queue is for band width simulation of caches/memories.
        CacheAccessRequestQueue* m_accessQueue;

        int m_level;                    // 何次キャッシュか
        std::string m_name;             // このキャッシュの名前

        // table information
        int m_indexBitSize;
        int m_offsetBitSize;
        int m_lineBitSize;
        int m_numWays;
        int m_numPorts;
        int m_reqQueueSize;
        int m_missedAccessListSize;

        // Statistic Information
        s64 m_numReadHit;           // Read時のヒット回数
        s64 m_numReadMiss;          // Read時のミス回数
        s64 m_numReadAccess;        // Readの回数
        s64 m_numReadPendingHit;    // PendingAccessにHitした回数

        s64 m_numPrefetchHit;       // Prefetch時のヒット回数
        s64 m_numPrefetchMiss;      // Prefetch時のミス回数
        s64 m_numPrefetchPendingHit;// PrefetchがPendingAccessにHitした回数
        s64 m_numPrefetchAccess;    // Prefetchの回数

        s64 m_numWriteHit;          // Write時のヒット回数
        s64 m_numWriteMiss;         // Write時のミス回数
        s64 m_numWriteAccess;       // Writeの回数
        s64 m_numWritePendingHit;   // PendingAccessにHitした回数

        s64 m_numWrittenBackLines;  // The number of cache lines written back

        s64 m_numInvalidated;       // The number of invalidated lines.
        s64 m_capacityKB;           // 容量(キロバイト)

        // Returns whether an access is prefetch or not.
        bool IsPrefetch( const Access& access );

        // Update statistics.
        void UpdateStatistics( const Access& access, Result::State state );

        // Check whether the access is valid or not
        void CheckValidAddress( const Addr& access );

        // Add accesses to a missed access list.
        void AddToMissedAccessList( 
            const Access& access,
            const Result& result,
            CacheAccessNotifieeIF* notifee,
            const NotifyParam& param
        );

        //
        // --- Read handler
        //

        // Processes on a cache hit.
        Result OnReadHit( const Access& access);

        // Processes on a cache partial hit (hits in PendingAccess).
        Result OnReadPendingHit( 
            const Access& access, 
            const Result& phResult,
            CacheAccessNotifieeIF* notifee 
        );

        // Processes on a cache miss
        Result OnReadMiss( 
            const Access& access, 
            CacheAccessNotifieeIF* notifee 
        );

        //
        // --- Write handler
        //

        // Processes on a cache hit.
        Result OnWriteHit( const Access& access);

        // Processes on a cache partial hit (hits in PendingAccess).
        Result OnWritePendingHit( 
            const Access& access, 
            const Result& phResult,
            CacheAccessNotifieeIF* notifee 
        );

        // Processes on a cache miss
        Result OnWriteMiss( 
            const Access& access,
            CacheAccessNotifieeIF* notifee 
        );

        // Write an access to the cache table.
        void UpdateTable( const Access& access );


        //
        // Implements
        //

        // Cache read
        void ReadBody( CacheHookParam* param );

        // Cache write
        // レイテンシは，ライトミス時にライトアロケートにかかった時間を含む
        void WriteBody( CacheHookParam* param );

        // Invalidate line
        void InvalidateBody( CacheHookParam* param );

        // Write an access to the cache table.
        void UpdateTableBody( CacheHookParam* param );

    };

}; // namespace Onikiri

#endif // SIM_MEMORY_CACHE_CACHE_H

