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
// ARQ: AccessRequestQueue
//   This is used for simulation of band width limit.
//
// MAL: MissedAccessList
//   This is used for missed access handling including 
//   write allocate and refill. 
//   MAL corresponds to a Miss Status Handling Register (MSHR).
//  
//   Upper caches
//--------|--------
//    ---------
//    |  ARQ  |
//    ---------
//        |
//    ---------
//    | Cache |
//    ---------
//        |
//    ---------
//    |  MAL  |
//    ---------
//--------|--------
//   Lower caches
//

#include <pch.h>

#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Op/Op.h"
#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Memory/Cache/CacheMissedAccessList.h"
#include "Sim/Memory/Prefetcher/PrefetcherIF.h"
#include "Sim/Memory/Cache/CacheExtraStateTable.h"
#include "Sim/Memory/Cache/CacheAccessRequestQueue.h"

//
// --- Hooks
//
namespace Onikiri
{
    HookPoint<Cache, CacheHookParam> Cache::s_readHook;
    HookPoint<Cache, CacheHookParam> Cache::s_writeHook;
    HookPoint<Cache, CacheHookParam> Cache::s_invalidateHook;
    HookPoint<Cache, CacheHookParam> Cache::s_tableUpdateHook;
};

using namespace Onikiri;

Cache::Cache() :
    m_latency           (0),
    m_nextLevelCache    (0),
    m_prefetcher        (0),
    m_perfect           (0),
    m_writePolicy       (WP_INVALID),
    m_missedAccessList  (0),
    m_accessQueue       (0),
    m_level             (0),
    m_indexBitSize      (0),
    m_offsetBitSize     (0),
    m_numWays           (0),
    m_numPorts          (0),
    m_reqQueueSize      (0),
    m_missedAccessListSize(0),
    m_numReadHit        (0),
    m_numReadMiss       (0),
    m_numReadAccess     (0),
    m_numReadPendingHit (0),
    m_numPrefetchHit    (0),
    m_numPrefetchMiss   (0),
    m_numPrefetchPendingHit(0),
    m_numPrefetchAccess (0),
    m_numWriteHit       (0),
    m_numWriteMiss      (0),
    m_numWriteAccess    (0),
    m_numWritePendingHit(0),
    m_numWrittenBackLines(0),
    m_numInvalidated    (0),
    m_capacityKB        (0)
{
    m_lineBitSize = 0;
    m_capacityKB = 0;
    m_cacheTable = 0;
    m_lineState = new ExtraStateTableType();
}

Cache::~Cache()
{
    ReleaseParam();

    if( m_lineState != NULL ){
        delete m_lineState;
        m_lineState = NULL;
    }

    if( m_missedAccessList != NULL ){
        delete m_missedAccessList;
        m_missedAccessList = NULL;
    }

    if( m_accessQueue != NULL ){
        delete m_accessQueue;
        m_accessQueue = NULL;
    }
}

void Cache::Initialize( InitPhase phase )
{
    PipelineNodeBase::Initialize( phase );

    if(phase == INIT_PRE_CONNECTION){

        LoadParam();
        m_lineBitSize = m_offsetBitSize;
        int lineSize = (1 << m_lineBitSize);

        m_capacityKB = 
            m_numWays * 
            (1 << m_indexBitSize) *
            lineSize /
            1024;

        // This cache can transfer one line for 'm_exclusiveAccessCycles' cycles.
        if( m_exclusiveAccessCycles == 0 || m_numPorts == 0 ){
            m_maxThroughputBytesPerCycle = "Infinite";
        }
        else{
            m_maxThroughputBytesPerCycle.format( 
                "%f", 
                (double)lineSize / (double)m_exclusiveAccessCycles * m_numPorts
            );
        }


        if( m_latency < m_exclusiveAccessCycles ){
            THROW_RUNTIME_ERROR(
                "The latency is less than the exclusive access cycles. "
                "This means that the cache/memory cannot transfer whole "
                "line data within the latency because the bandwidth "
                "is too narow."
            );
        };

        if( m_numPorts != 0 ){
            if( m_exclusiveAccessCycles*m_reqQueueSize/m_numPorts + m_latency >
                GetLowerPipeline()->GetWheelSize()
            ){
                RUNTIME_WARNING( 
                    "The size of the time wheel may not be enough. " 
                    "Inclease the time wheel size or decrease @ExclusiveAccessCycles/@NumRequestListSize "
                );
            }
        }

        m_cacheTable =
            new Cache::TableType( 
                HasherType( m_indexBitSize, m_offsetBitSize ), 
                m_numWays 
            );

    }
    else if( phase == INIT_POST_CONNECTION ){
        
        int offset = m_offsetBitSize;

        m_missedAccessList = 
            new CacheMissedAccessList( GetLowerPipeline(), offset );

        m_accessQueue = 
            new CacheAccessRequestQueue( GetLowerPipeline(), m_numPorts, m_exclusiveAccessCycles );

        if( m_prevLevelCaches.GetSize() == 0 && 
            ( m_exclusiveAccessCycles != 0 || m_numPorts != 0)
        ){
            // Scheduler assumes that a top level cache(L1) has enough band width.
            // If an user enable band width limit option of a top level cache, 
            // the system will be dead locked...
            THROW_RUNTIME_ERROR( "Band width limit of a top level cache is not support." );
        }

        if( !m_perfect && !m_nextLevelCache ){
            THROW_RUNTIME_ERROR( "The last level of a memory hierarchy must be 'perfect'." );
        }

        LineState initState;
        initState.dirty = false;
        m_lineState->Resize( this, initState );

        DisableLatch();
    }
}

// This method is called when a simulation mode is changed.
void Cache::ChangeSimulationMode( PhysicalResourceNode::SimulationMode mode )
{
    ASSERT( m_missedAccessList );
    ASSERT( m_accessQueue );

    bool isSimulation = ( mode == PhysicalResourceNode::SM_SIMULATION );

    // Disable access queues if the 'mode' is not 'Simulation' because 
    // Access queues use event mechanisms, which can run only under 'Simulation' mode.
    // In disabled queues, pushed accesses are reflected to caches immediately.
    m_missedAccessList->SetEnabled( isSimulation );
    m_accessQueue->SetEnabled(  isSimulation );
}


// Update statistics.
void Cache::UpdateStatistics( const Access& access, Result::State state )
{
    switch( access.type ){

    case AOT_READ:
    case AOT_READ_FOR_WRITE_ALLOCATE:
        m_numReadAccess++;
        switch( state ){
        case Result::ST_HIT:            m_numReadHit++;         break;
        case Result::ST_PENDING_HIT:    m_numReadPendingHit++;  break;
        case Result::ST_MISS:           m_numReadMiss++;        break;
        case Result::ST_NOT_ACCESSED:   ASSERT(0);              break;
        }
        break;

    case AOT_WRITE:
    case AOT_WRITE_BACK:
        m_numWriteAccess++;
        switch( state ){
        case Result::ST_HIT:            m_numWriteHit++;        break;
        case Result::ST_PENDING_HIT:    m_numWritePendingHit++; break;
        case Result::ST_MISS:           m_numWriteMiss++;       break;
        case Result::ST_NOT_ACCESSED:   ASSERT(0);              break;
        }
        break;
    
    case AOT_PREFETCH:
        m_numPrefetchAccess++;
        switch( state ){
        case Result::ST_HIT:            m_numPrefetchHit++;         break;
        case Result::ST_PENDING_HIT:    m_numPrefetchPendingHit++;  break;
        case Result::ST_MISS:           m_numPrefetchMiss++;        break;
        case Result::ST_NOT_ACCESSED:   ASSERT(0);                  break;
        }
        break;

    }
}


// Chech whether the access is valid or not
void Cache::CheckValidAddress( const Addr& addr )
{
    ASSERT(
        addr.pid != PID_INVALID && addr.tid != TID_INVALID,
        "The address has an invalid id (PID:%d, TID:%d).",
        addr.pid, addr.tid
    );
}


// Add accesses to a missed access list.
void Cache::AddToMissedAccessList( 
    const Access& access,
    const Result& result,
    CacheAccessNotifieeIF* notifee,
    const NotifyParam& param
){
    CacheAccessNotifieeIF* notifeeList[2] = { this, notifee };
    int notifieeCount = notifee != NULL ? 2 : 1;

    // Add the missed access to the list.
    m_missedAccessList->Add( 
        access, 
        result, 
        notifeeList,    // Notifiees
        notifieeCount,  // The number of notifiees
        param   
    );
}

// Get the access latency of a hit access.
Cache::Result Cache::OnReadHit( const Access& access)
{
    // Update cache replacement information.
    if( !m_perfect ){
        m_cacheTable->read( access.address );
    }

    Result result( (int)m_latency, Result::ST_HIT, this );

    // Push the access to the queue.
    result.latency = 
        (int)m_accessQueue->Push( 
            access, 
            m_latency,
            NULL,
            NotifyParam()
        );
    
    return result;
}


// Process accesses that hit in the missed access list.
Cache::Result Cache::OnReadPendingHit( 
    const Access& access, 
    const Result& phResult,
    CacheAccessNotifieeIF* notifee
){
    ASSERT( !m_perfect );

    Result result ( phResult.latency, Result::ST_PENDING_HIT, this );
    if( result.latency < m_latency ){
        result.latency = m_latency;
    }

    NotifyParam notification( CAET_FILL_FROM_MAL_FINISHED, result );
    AddToMissedAccessList( access, result, notifee, notification );

    return result;
}


// Processes on cache misses.
Cache::Result Cache::OnReadMiss( 
    const Access& access,
    CacheAccessNotifieeIF* notifee
){
    // 次のレベルのキャッシュにレイテンシを問い合わせる
    Result nextResult = m_nextLevelCache->Read( access, NULL );
    int nextLatency = nextResult.latency;

    Result result( m_latency + nextLatency, Result::ST_MISS, nextResult.cache );

    NotifyParam notification( CAET_FILL_FROM_NEXT_CACHE_FINISHED, result );
    AddToMissedAccessList( access, result, notifee, notification );

    return result;
}


// リードしてレイテンシを返す
// A implementation body of Read/ReadInorder.
void Cache::ReadBody( CacheHookParam* param )
{
    const Access& access = *param->access;

    Addr addr = access.address;
    CheckValidAddress( addr );

    Result result;

    if( m_perfect ){
        // Perfect
        result = OnReadHit( access );
    } 
    else{
        CacheTableIterator line = m_cacheTable->find( addr );
        if( line != m_cacheTable->end() ){
            // Perfect or Hit
            result = OnReadHit( access );
            param->line = line;
        } 
        else{

            Result phResult = m_missedAccessList->Find( addr );
            CacheAccessNotifieeIF* notifee = param->notifiee;
            if( phResult.state == Result::ST_HIT ){ 
                // Hit in the missed access list.
                result = OnReadPendingHit( access, phResult, notifee );
            }
            else{
                // Miss
                result = OnReadMiss( access, notifee );
            }
        }
    }

    // Update statistics
    UpdateStatistics( access, result.state );
    param->result = result;
}

Cache::Result Cache::Read( const Access& access, NotifieeIF* notifiee )
{
    CacheHookParam param;
    param.access = &access;
    param.table  = m_cacheTable;
    param.address   = &access.address;
    param.notifiee = notifiee;
    param.line = m_cacheTable->end();

    HookEntry( this, &Cache::ReadBody, &s_readHook, &param );

    if( m_prefetcher ){
        m_prefetcher->OnCacheRead( this, &param );
    }

    return param.result;
}


// Processes on a cache hit.
Cache::Result Cache::OnWriteHit( const Access& access )
{
    Result result( (int)m_latency, Result::ST_HIT, this );
    NotifyParam notification( CAET_WRITE_ACCESS_FINISHED, result );

    // Push the access to the queue.
    result.latency = 
    (int)m_accessQueue->Push( 
        access, 
        m_latency,
        this,
        notification
    );

    return result;  
}

// Processes on a cache partial hit (hits in PendingAccess).
Cache::Result Cache::OnWritePendingHit( 
    const Access& access, 
    const Result& phResult,
    CacheAccessNotifieeIF* notifee
){
    Access writeAllocate = access;
    writeAllocate.type = Access::OT_READ_FOR_WRITE_ALLOCATE;

    // Add the access to the missed accessed list.
    NotifyParam notification( CAET_WRITE_ALLOCATE_FINISHED, phResult );
    AddToMissedAccessList( writeAllocate, phResult, notifee, notification );

    return Result( 
        m_latency + phResult.latency, 
        Result::ST_MISS,
        this
    );
}

// Processes on a cache miss
Cache::Result Cache::OnWriteMiss( 
    const Access& access,
    CacheAccessNotifieeIF* notifee
){
    // Write allocate
    Access writeAllocate = access;
    writeAllocate.type = Access::OT_READ_FOR_WRITE_ALLOCATE;
    Result nextResult = 
        m_nextLevelCache->Read( writeAllocate, NULL );

    int nextLatency = nextResult.latency;
    
    NotifyParam notification( CAET_WRITE_ALLOCATE_FINISHED, nextResult );
    AddToMissedAccessList( writeAllocate, nextResult, notifee, notification );

    return Result( 
        m_latency + nextLatency, 
        Result::ST_MISS,
        nextResult.cache
    );
}


// StoreのRetire時にm_cacheにWriteをする
// レイテンシは，ライトミス時にライトアロケートにかかった時間を含む
Cache::Result Cache::Write( const Access& access, NotifieeIF* notifiee )
{
    CacheHookParam param;
    param.access = &access;
    param.address   = &access.address;
    param.table  = m_cacheTable;
    param.notifiee = notifiee;
    param.line = m_cacheTable->end();

    HookEntry( this, &Cache::WriteBody, &s_writeHook, &param );

    if( m_prefetcher ){
        m_prefetcher->OnCacheWrite( this, &param );
    }

    return param.result;
}

void Cache::WriteBody( CacheHookParam* param )
{
    const Access& access = *param->access;

    Addr addr = access.address;
    CheckValidAddress( addr );

    Result result;

    if( m_perfect ){
        // Perfect
        result = OnWriteHit( access );
    }
    else{

        CacheTableIterator line = m_cacheTable->find( addr );
        if( line != m_cacheTable->end() ){
            // Hit
            result = OnWriteHit( access );
            param->line = line;
        }
        else{
            CacheAccessNotifieeIF* notifee = param->notifiee;
            Result phResult = m_missedAccessList->Find( addr );
            if( phResult.state == Result::ST_HIT ){ 
                // Write allocate request hits in the missed access list.
                result = OnWritePendingHit( access, phResult, notifee );
            }
            else{   
                // Miss
                result = OnWriteMiss( access, notifee );
            }
        }
    }

    // Update statistics
    UpdateStatistics( access, result.state );

    param->result = result;
}

// Write an access to the cache table on re-fill, write, write-back and write-allocate.
void Cache::UpdateTableBody( CacheHookParam* param )
{
    // There is no need to update set-assoc table if perfect
    if (m_perfect) {
        return;
    }

    const CacheAccess& access = *param->access;
    bool dirty = access.IsWirte();

    bool replaced;
    CacheLine replacedLine;
    Addr replacedAddr;
    TableType::iterator line =
        m_cacheTable->write( access.address, Value(), &replaced, &replacedAddr, &replacedLine );
    param->line = line;
    param->replaced = replaced;

    if( replaced ){

        // Perfect cache must not invalidate upper cache lines.
        if (!m_perfect) {
            // Invalidate replaced lines in previous level caches for keep inclusion.
            for( int i = 0; i < m_prevLevelCaches.GetSize(); i++ ){
                m_prevLevelCaches[i]->Invalidate( replacedAddr );
            }
        }
        
        // Write back replaced lines to the next level cache.
        if( m_writePolicy == WP_WRITE_BACK && !m_perfect ){
            if( (*m_lineState)[ line ].dirty ){
                Access writeBack  = access;
                writeBack.address = replacedAddr;
                writeBack.value       = 0;
                writeBack.lineValue   = replacedLine.value;
                writeBack.type    = AOT_WRITE_BACK;
                m_nextLevelCache->Write( writeBack, NULL );
                m_numWrittenBackLines++;
            }
        }
    }

    // Set a dirty flag.
    (*m_lineState)[ line ].dirty = dirty;
}


void Cache::UpdateTable( const Access& access )
{
    CacheHookParam param;
    param.access = &access;
    param.table  = m_cacheTable;
    param.line = m_cacheTable->end();

    HookEntry( this, &Cache::UpdateTableBody, &s_tableUpdateHook, &param );

    if( m_prefetcher ){
        m_prefetcher->OnCacheTableUpdate( this, &param );
    }
}


// Invalidate line
void Cache::Invalidate( const Addr& addr )
{
    CacheHookParam param;
    param.address = &addr;
    param.table  = m_cacheTable;
    param.line = m_cacheTable->end();

    HookEntry( this, &Cache::InvalidateBody, &s_invalidateHook, &param );

    if( m_prefetcher ){
        m_prefetcher->OnCacheInvalidation( this, &param );
    }
}

void Cache::InvalidateBody( CacheHookParam* param )
{
    CacheTableIterator line = m_cacheTable->invalidate( *param->address );
    if( line != m_cacheTable->end() ){
        m_numInvalidated++;
    }
    param->line = line;
}

// PendingAccess から各種アクセス終了の通知をうける
void Cache::AccessFinished( const Access& access, const NotifyParam& param ) 
{
    CheckValidAddress( access.address );

    switch( param.type ){
    case CAET_FILL_FROM_MAL_FINISHED:
        ASSERT( !access.IsWirte() );
        break;

    case CAET_FILL_FROM_NEXT_CACHE_FINISHED:
        ASSERT( !access.IsWirte() );
        UpdateTable( access );
        break;
    
    case CAET_WRITE_ALLOCATE_FINISHED:
    {
        ASSERT( !access.IsWirte() );
        Access writeAccess = access;
        writeAccess.type = Access::OT_WRITE;
        OnWriteHit( writeAccess );
        break;
    }

    case CAET_WRITE_ACCESS_FINISHED:
        ASSERT( access.IsWirte() );
        UpdateTable( access );
        if( m_writePolicy == WP_WRITE_THROUGH && !m_perfect ){
            m_nextLevelCache->Write( access, NULL );
        }
        break;

    default:
        ASSERT(0);
        break;
    }
    
}


Cache* Cache::GetNextCache()
{
    if( m_nextLevelCache != 0 ){
        return m_nextLevelCache;
    } 
    else{
        return 0;
    }
}

void Cache::SetNextCache( PhysicalResourceArray<Cache>& next )
{
    m_nextLevelCache = next[0];
    m_nextLevelCache->AddPreviousLevelCache( this );
}

// Add previous level cache
void Cache::AddPreviousLevelCache( Cache* prev )
{
    for( int i = 0; i < m_prevLevelCaches.GetSize(); i++ ){
        if( m_prevLevelCaches[i] == prev ){
            THROW_RUNTIME_ERROR( "A same previous level cache is added more than once." );
        }
    }

    m_prevLevelCaches.Add( prev );
}

int Cache::GetOffsetBitSize() const
{
    return m_offsetBitSize;
}

// Returns true when resources (a request queue etc) are full and stall is required.
bool Cache::IsStallRequired()
{

    if( m_nextLevelCache && m_nextLevelCache->IsStallRequired() ){
        return true;
    }

    if( m_reqQueueSize != 0 && 
        (int)m_accessQueue->GetSize() >= m_reqQueueSize 
    ){
        return true;
    }

    if( m_missedAccessListSize != 0 && 
        (int)m_missedAccessList->GetSize() >= m_missedAccessListSize 
    ){
        return true;
    }

    return false;
}

// This is called on every cycle except stall time.
void Cache::Update()
{
}
